/// @file analogInputs.c

#include "sharedData.h"

#include "debuggingTools.h"

#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

//---------------------------------------------------------------------------------------------------
// Macro directives
//---------------------------------------------------------------------------------------------------

/// This is size of the buffer for raw samples; it must be a power of 2 for the correct operation of the circular buffer
#define ADC_RAW_BUFFER_SIZE 4

/// This mask is used for calculating the index in the circular buffer for raw samples
#define ADC_INDEX_MASK 3

#define ANALOG_MAX_CHANNELS 4

// This directive indicates that the ADC0 and ADC1 on RP2040 are assigned to GPIO26 and GPIO27 ports respectively
#define GPIO_FOR_ADC0 26
#define GPIO_FOR_ADC1 27

#define GPIO_FOR_CUP_MULTIPLEXER_CONTROL_0 10
#define GPIO_FOR_CUP_MULTIPLEXER_CONTROL_1 9

#define GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_0 12
#define GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_1 11

#define ADC_VALUE_LIMIT 3200u	// experimentaly determined value
#define COEFFICIENT_LIMIT 300000u	// experimentaly determined value
#define CALCULATION_RESULT_MAX 32000 // experimentaly determined value
#define CALCULATION_RESULT_MIN 320 // experimentaly determined value (?)

#define TEMPORARY_RESCALING 0x4000 // must be a power of 2 to optimize the calculation

#define ADC_SIGNAL_TOO_LARGE 1u
#define ADC_INVALID_CALIBRATION_DATA 2u
#define ADC_OTHER_CALCULATION_ERROR 4u
#define ADC_YET_ANOTHER_CALCULATION_ERROR 8u

#define FILTER_COEFFICIENT_A 0.05
#define FILTER_COEFFICIENT_B (1.0 - FILTER_COEFFICIENT_A)

//---------------------------------------------------------------------------------------------------
// Data structures
//---------------------------------------------------------------------------------------------------

typedef struct {
	uint16_t cup_number;
	uint16_t channel_number;
	uint32_t accumulator;
	uint16_t x_a;
	uint16_t x_b;
	uint16_t y_a;
	uint16_t y_b;
	bool is_signal_large;
} CalculationsInputDataStruct;

//---------------------------------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------------------------------

bool IirFilterReset = false;

//---------------------------------------------------------------------------------------------------
// Local constants
//---------------------------------------------------------------------------------------------------

static const float GetVoltageCoefficient = (float)1.0 / ((float)ADC_RAW_BUFFER_SIZE);
static const float GetVoltageOffset = (float)0.0;

//---------------------------------------------------------------------------------------------------
// Local variables
//---------------------------------------------------------------------------------------------------

/// @brief This is a buffer for raw samples from the ADC0 converter
static uint16_t RawBufferAdc0[ANALOG_MAX_CHANNELS][ADC_RAW_BUFFER_SIZE];

/// @brief This is a buffer for raw samples from the ADC1 converter
static uint16_t RawBufferAdc1[ANALOG_MAX_CHANNELS][ADC_RAW_BUFFER_SIZE];

/// @brief Index for writing new samples from ADC0 and ADC1
static uint32_t AdcBuffersHead = 0;

/// @brief This variable stores the value of the active cup (1, 2 or 3) that is read from Modbus holding register; 
/// it is used for checking if the active cup has changed since the last measurement cycle
static uint16_t LocalActiveCup = 0xFFFFu;

/// @brief This variable stores the index of the currently measured channel
static uint16_t ActiveChannel;

static bool IsSignalLarge[ANALOG_MAX_CHANNELS] = {true, true, true, true};

static CalculationsInputDataStruct CalculationsTemporaryData;

static bool PrintoutsForTestingPurposes;
static double FilteredValues0[ANALOG_MAX_CHANNELS];
static double FilteredValues1[ANALOG_MAX_CHANNELS];

//---------------------------------------------------------------------------------------------------
// Local function prototypes
//---------------------------------------------------------------------------------------------------

static void controlSelectedCup( uint16_t CupNumber );
static void controlSelectedChannel( uint16_t ChannelNumber );

//---------------------------------------------------------------------------------------------------
// Function definitions
//---------------------------------------------------------------------------------------------------

/// This function initializes peripherals for ADC measuring and the local variables related to ADC measurements
void initializeAdcMeasurements(void) {
	AdcBuffersHead = 0;
	adc_init();
	adc_gpio_init(GPIO_FOR_ADC0);
	adc_gpio_init(GPIO_FOR_ADC1);

	gpio_init(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_0);
	gpio_set_dir(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_0, GPIO_OUT);
	gpio_put(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_0, false);

	gpio_init(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_1);
	gpio_set_dir(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_1, GPIO_OUT);
	gpio_put(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_1, false);

	gpio_init(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_0);
	gpio_set_dir(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_0, GPIO_OUT);
	gpio_put(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_0, false);

	gpio_init(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_1);
	gpio_set_dir(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_1, GPIO_OUT);
	gpio_put(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_1, false);

	ActiveChannel = 0;
}

/// @brief This function collects measurements from ADC
/// It should be called only by the timer ISR (repeatingTimerISR)
void analogInputsMeasurements(void) {
	AdcBuffersHead &= ADC_INDEX_MASK;
	// Measure ADC0
	adc_select_input(0);
	(void)adc_read(); // dummy read
	RawBufferAdc0[ActiveChannel][AdcBuffersHead] = adc_read();
	// Measure ADC1
	adc_select_input(1);
	(void)adc_read(); // dummy read
	RawBufferAdc1[ActiveChannel][AdcBuffersHead] = adc_read();
	// Update the index for the next samples
	AdcBuffersHead++;

	uint16_t SafeActiveCup = (LocalActiveCup <= 2u) ? LocalActiveCup : 0u;

	if (ActiveChannel < (ANALOG_MAX_CHANNELS - 1)) {
		ActiveChannel++;
	} else {
		ActiveChannel = 0;

			// Keep local cup state synchronized with the debug argument register.
			if (LocalActiveCup + 1u != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_ARGUMENT1)])
		{
			if (0 == ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_ARGUMENT1)]){
				ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_ARGUMENT1)] = 1u; // just for testing purposes (initialization of the register)
			}
			LocalActiveCup = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_ARGUMENT1)];
			LocalActiveCup--;
			if (LocalActiveCup > 2u){
				LocalActiveCup = 0u;
			}
			SafeActiveCup = LocalActiveCup;

			for (uint16_t J = 0; J < ANALOG_MAX_CHANNELS; J++) {
				for (uint16_t K = 0; K < ADC_RAW_BUFFER_SIZE; K++) {
					RawBufferAdc0[J][K] = 0;
					RawBufferAdc1[J][K] = 0;
				}
				IsSignalLarge[J] = true; // initial value for the first measurement cycle after the cup change
			}

			for (int J = 0; J < MODBUS_INPUT_REGISTERS_NUMBER; J++) {
				ModbusInputRegisters[J] = 0;
			}
		}

		// just for testing purposes
		PrintoutsForTestingPurposes  = ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 1u) != 0u);

		// Defensive clamp to avoid out-of-bounds writes when local state is invalid.
		SafeActiveCup = (LocalActiveCup <= 2u) ? LocalActiveCup : 0u;

		// Calculations are carried out on the basis of the samples stored in the buffers, 
		// and the results are stored in Modbus input registers
		static uint16_t CalculationDivider = 0;
		CalculationDivider++;
		CalculationDivider &= 3u;
		if (CalculationDivider == 0) {
			if (PrintoutsForTestingPurposes) {
				printf("Cup %u ", SafeActiveCup + 1u); // just for testing purposes
			}
			for (uint16_t Channel = 0; Channel < ANALOG_MAX_CHANNELS; Channel++) {
				uint16_t HoldingBaseIndexX = holdingIndexFromAddress(MODBUS_CALIBRATION_X_REGISTERS_ADDRESS + (SafeActiveCup * (4u*6u)) + (Channel * 6u));
				uint16_t HoldingBaseIndexY = holdingIndexFromAddress(MODBUS_CALIBRATION_Y_REGISTERS_ADDRESS);
				uint32_t AccumulatorHighGain = 0;
				uint32_t AccumulatorLowGain = 0;

				auxiliaryPinOutputValue1(true); // just for debugging purposes

				for (uint16_t K = 0; K < ADC_RAW_BUFFER_SIZE; K++) {
					AccumulatorHighGain += RawBufferAdc0[Channel][K];
					AccumulatorLowGain += RawBufferAdc1[Channel][K];
				}
				if (IsSignalLarge[Channel]) {
					if (AccumulatorLowGain < ModbusHoldingRegisters[HoldingBaseIndexX + 2u]){
						IsSignalLarge[Channel] = false;
					}
				} else {
					if (AccumulatorLowGain > ModbusHoldingRegisters[HoldingBaseIndexX + 1u]){
						IsSignalLarge[Channel] = true;
					}
				}
				CalculationsTemporaryData.cup_number = SafeActiveCup;
				CalculationsTemporaryData.channel_number = Channel;
				if (IsSignalLarge[Channel]) {
					uint16_t X1 = ModbusHoldingRegisters[HoldingBaseIndexX];
					uint16_t X2 = ModbusHoldingRegisters[HoldingBaseIndexX+1u];
					uint16_t X3 = ModbusHoldingRegisters[HoldingBaseIndexX+2u];

					CalculationsTemporaryData.accumulator = AccumulatorLowGain;
					CalculationsTemporaryData.is_signal_large = true;
					if (AccumulatorLowGain > X2) {
						CalculationsTemporaryData.x_a = X1;
						CalculationsTemporaryData.x_b = X2;
						CalculationsTemporaryData.y_a = ModbusHoldingRegisters[HoldingBaseIndexY];
						CalculationsTemporaryData.y_b = ModbusHoldingRegisters[HoldingBaseIndexY+1u];
					} else {
						CalculationsTemporaryData.x_a = X2;
						CalculationsTemporaryData.x_b = X3;
						CalculationsTemporaryData.y_a = ModbusHoldingRegisters[HoldingBaseIndexY+1u];
						CalculationsTemporaryData.y_b = ModbusHoldingRegisters[HoldingBaseIndexY+2u];
					}
				} else {
					uint16_t X1 = ModbusHoldingRegisters[HoldingBaseIndexX+3u];
					uint16_t X2 = ModbusHoldingRegisters[HoldingBaseIndexX+4u];
					uint16_t X3 = ModbusHoldingRegisters[HoldingBaseIndexX+5u];

					CalculationsTemporaryData.accumulator = AccumulatorHighGain;
					CalculationsTemporaryData.is_signal_large = false;
					if (AccumulatorHighGain >= X2) {
						CalculationsTemporaryData.x_a = X1;
						CalculationsTemporaryData.x_b = X2;
						CalculationsTemporaryData.y_a = ModbusHoldingRegisters[HoldingBaseIndexY+1u];
						CalculationsTemporaryData.y_b = ModbusHoldingRegisters[HoldingBaseIndexY+2u];
					} else {
						CalculationsTemporaryData.x_a = X2;
						CalculationsTemporaryData.x_b = X3;
						CalculationsTemporaryData.y_a = ModbusHoldingRegisters[HoldingBaseIndexY+2u];
						CalculationsTemporaryData.y_b = ModbusHoldingRegisters[HoldingBaseIndexY+3u];
					}
				}


				auxiliaryPinOutputValue1(false); // just for debugging purposes

				int32_t Result = 0;
				uint16_t ErrorCode = 0;
				if (CalculationsTemporaryData.accumulator > ADC_VALUE_LIMIT){
					Result = UINT16_MAX;
					ErrorCode = ADC_SIGNAL_TOO_LARGE;
				}
				uint32_t Coefficient = 0;
				if (0 == ErrorCode) {
					Coefficient = (uint32_t)(CalculationsTemporaryData.y_a - CalculationsTemporaryData.y_b) * (uint32_t)TEMPORARY_RESCALING;
					Coefficient /= (uint32_t)(CalculationsTemporaryData.x_a - CalculationsTemporaryData.x_b);
					if ((Coefficient > COEFFICIENT_LIMIT) || (CalculationsTemporaryData.x_a <= CalculationsTemporaryData.x_b) || 
					(CalculationsTemporaryData.y_a <= CalculationsTemporaryData.y_b)) 
					{
						Result = UINT16_MAX;
						ErrorCode = ADC_INVALID_CALIBRATION_DATA;
					}
					if (0 == ErrorCode) {
						Result = ((int32_t)Coefficient * ((int32_t)CalculationsTemporaryData.accumulator - (int32_t)CalculationsTemporaryData.x_b));
						Result = Result / (int32_t)TEMPORARY_RESCALING + (int32_t)CalculationsTemporaryData.y_b;
						if (Result > CALCULATION_RESULT_MAX) {
							Result = UINT16_MAX;
							ErrorCode = ADC_OTHER_CALCULATION_ERROR;
						}
						if (Result < -CALCULATION_RESULT_MIN) {
							Result = UINT16_MAX;
							ErrorCode = ADC_YET_ANOTHER_CALCULATION_ERROR;
						}
						if (Result < 0) {
							Result = 0;
						}
					}
				}
				ModbusInputRegisters[SafeActiveCup*5 + Channel] = (uint16_t)Result;

				auxiliaryPinOutputValue1(true); // just for debugging purposes

				// just for testing purposes
				if (PrintoutsForTestingPurposes) {
					if (IirFilterReset) {
						FilteredValues0[Channel] = (double)AccumulatorHighGain;
						FilteredValues1[Channel] = (double)AccumulatorLowGain;
					}
					FilteredValues0[Channel] = (FilteredValues0[Channel] * FILTER_COEFFICIENT_B) + ((double)AccumulatorHighGain * FILTER_COEFFICIENT_A);
					FilteredValues1[Channel] = (FilteredValues1[Channel] * FILTER_COEFFICIENT_B) + ((double)AccumulatorLowGain * FILTER_COEFFICIENT_A);

					uint16_t SelectedChannel = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_ARGUMENT2)];
					if (0 == SelectedChannel) {
						printf("Ch%u: %4lu [%4lu] %3lu [%4lu] %u.%02u uA %u|", Channel, 
								AccumulatorHighGain, (uint32_t)(FilteredValues0[Channel]+0.5f), 
								AccumulatorLowGain, (uint32_t)(FilteredValues1[Channel]+0.5f), 
								(unsigned int)(ModbusInputRegisters[SafeActiveCup*5 + Channel]/100), 
								(unsigned int)(ModbusInputRegisters[SafeActiveCup*5 + Channel]%100),
								ErrorCode);
					}
					if (SelectedChannel == (Channel + 1u)) {
						printf("Ch%u: %4lu [%4lu] %3lu [%4lu] %u.%02u uA %c%u | x1=%u x2=%u y1=%u y2=%u | coef=%lu res=%ld | BaseIndexX=%u BaseIndexY=%u", Channel, 
								AccumulatorHighGain, (uint32_t)(FilteredValues0[Channel]+0.5f), 
								AccumulatorLowGain, (uint32_t)(FilteredValues1[Channel]+0.5f), 
								(unsigned int)(ModbusInputRegisters[SafeActiveCup*5 + Channel]/100), 
								(unsigned int)(ModbusInputRegisters[SafeActiveCup*5 + Channel]%100),
								IsSignalLarge[Channel] ? 'L' : 'H',										// L = low gain, H = high gain
								ErrorCode,
								CalculationsTemporaryData.x_a, CalculationsTemporaryData.x_b,
								CalculationsTemporaryData.y_a, CalculationsTemporaryData.y_b,
								Coefficient, Result,
								HoldingBaseIndexX, HoldingBaseIndexY);
					}
				} else {
					FilteredValues0[Channel] = (double)AccumulatorHighGain;
					FilteredValues1[Channel] = (double)AccumulatorLowGain;
				}



				auxiliaryPinOutputValue1(false); // just for debugging purposes



			}
			IirFilterReset = false;
			if (PrintoutsForTestingPurposes) {
				printf("\r\n"); // just for testing purposes
			}
		}
	}
	controlSelectedCup(SafeActiveCup);
	controlSelectedChannel(ActiveChannel);
}

/// @brief This function measures the voltage at ADC input and make some calculations
/// The function should be called only in the main loop
float getVoltage( uint16_t ChannelNumber ) {
	uint32_t Accumulator0 = 0;
	uint32_t Accumulator1 = 0;
	bool IsSignalLarge = false;
	for (uint8_t J = 0; J < ADC_RAW_BUFFER_SIZE; J++) {
		Accumulator0 += RawBufferAdc0[ChannelNumber][J];
		Accumulator1 += RawBufferAdc1[ChannelNumber][J];
	}
	return (float)Accumulator0 * GetVoltageCoefficient - GetVoltageOffset;
}

static void controlSelectedCup( uint16_t CupNumber ){
	gpio_put(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_0, (CupNumber & 1u) == 0u);
	gpio_put(GPIO_FOR_CUP_MULTIPLEXER_CONTROL_1, (CupNumber & 2u) == 0u);
}

static void controlSelectedChannel( uint16_t ChannelNumber ){
	gpio_put(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_0, (ChannelNumber & 1u) == 0u);
	gpio_put(GPIO_FOR_CHANNEL_MULTIPLEXER_CONTROL_1, (ChannelNumber & 2u) == 0u);
}