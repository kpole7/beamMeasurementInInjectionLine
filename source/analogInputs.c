/// @file analogInputs.c

#include "sharedData.h"
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

//---------------------------------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------------------------------

uint16_t AnalogRangeChangeThreshold = DEFAULT_ANALOG_RANGE_CHANGE_THRESHOLD;

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
void getVoltageSamples(void) {
	// Step A . . . . . . . . . .
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

	// Step B . . . . . . . . . .



	// Step F . . . . . . . . . .
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
			}

			for (int J = 0; J < MODBUS_INPUT_REGISTERS_NUMBER; J++) {
				ModbusInputRegisters[J] = 0;
			}
		}

			// just for testing purposes
			bool PrintoutsForTestingPurposes =
				((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 1u) != 0u);

			// Defensive clamp to avoid out-of-bounds writes when local state is invalid.
			SafeActiveCup = (LocalActiveCup <= 2u) ? LocalActiveCup : 0u;

		// Calculations are carried out on the basis of the samples stored in the buffers, 
		// and the results are stored in Modbus input registers
		static uint16_t CalculationDivider = 0;
		CalculationDivider++;
		CalculationDivider &= 3u;
		if (CalculationDivider == 0) {
				if (PrintoutsForTestingPurposes) {
					printf("Cup: %u  ", SafeActiveCup + 1u); // just for testing purposes
			}
			for (uint16_t Channel = 0; Channel < ANALOG_MAX_CHANNELS; Channel++) {
				uint16_t Offset;
				uint16_t Factor;
				uint32_t Accumulator0 = 0;
				uint32_t Accumulator1 = 0;
				int32_t Result;
				int32_t SignedOffset = 0;
				char HighLowIndicator = ' '; // just for testing purposes
				for (uint16_t K = 0; K < ADC_RAW_BUFFER_SIZE; K++) {
					Accumulator0 += RawBufferAdc0[Channel][K];
					Accumulator1 += RawBufferAdc1[Channel][K];
				}
					if (Accumulator1 < ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_RANGE_CHANGE_THRESHOLD)]) {
						Offset = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CH1_GAIN1_OFFSET) + SafeActiveCup*4 + Channel];
						Factor = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CH1_GAIN1_FACTOR) + SafeActiveCup*4 + Channel];
					Result = (int32_t)Accumulator0;
					HighLowIndicator = 'H'; // just for testing purposes
				} else {
						Offset = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CH1_GAIN2_OFFSET) + SafeActiveCup*4 + Channel];
						Factor = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CH1_GAIN2_FACTOR) + SafeActiveCup*4 + Channel];
					Result = (int32_t)(Accumulator1*10u); // unify units
					HighLowIndicator = 'L'; // just for testing purposes
				}
				if (Offset >= 0x8000u) {
					SignedOffset = (int32_t)(Offset - 0x10000u);
				} else {
					SignedOffset = (int32_t)Offset;
				}
				Result += SignedOffset;
				Result = (Result * (int32_t)Factor) / 100000; // unit = 100nA

					if (Result < 0) {
						ModbusInputRegisters[SafeActiveCup*5 + Channel] = 0u;
				}
				else{
						ModbusInputRegisters[SafeActiveCup*5 + Channel] = (uint16_t)Result;
				}

				// just for testing purposes
				if (PrintoutsForTestingPurposes) {
					printf("Ch%u: %4lu %3lu %u.%u uA  ", Channel, Accumulator0, Accumulator1, 
							(unsigned int)(ModbusInputRegisters[SafeActiveCup*5 + Channel]/10), 
							(unsigned int)(ModbusInputRegisters[SafeActiveCup*5 + Channel]%10));
					if (Channel == 3u){
						printf("  %c  %d.%d uA  Offs= %ld  params: %u %u", HighLowIndicator, (int)(Result/10), (int)(Result%10), SignedOffset, Offset, Factor);
					}
				}

			}
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