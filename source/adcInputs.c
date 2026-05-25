/// @file adcInputs.c

#include "adcInputs.h"
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
static uint16_t LocalActiveCup;

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

	LocalActiveCup = 1u; // default value of the ActiveCup register
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

	if (ActiveChannel < (ANALOG_MAX_CHANNELS - 1)) {
		ActiveChannel++;
	} else {
		ActiveChannel = 0;

		static uint16_t RegisterOldValue; // just for testing purposes

		if (RegisterOldValue != ModbusHoldingRegisters[0x50]){	// debugging: 0x1050
			RegisterOldValue = ModbusHoldingRegisters[0x50];

			LocalActiveCup = RegisterOldValue; // just for testing purposes
			LocalActiveCup &= 0x7u;
			LocalActiveCup--;
			LocalActiveCup &= 3u;

			for (uint16_t J = 0; J < ANALOG_MAX_CHANNELS; J++) {
				for (uint16_t K = 0; K < ADC_RAW_BUFFER_SIZE; K++) {
					RawBufferAdc0[J][K] = 0;
					RawBufferAdc1[J][K] = 0;
				}
			}

			controlSelectedCup(LocalActiveCup);
		}

		// just for testing purposes
		static uint16_t PrintoutsDivider = 0;
		PrintoutsDivider++;
		PrintoutsDivider &= 3u;

		if (PrintoutsDivider == 0) {
			printf("Cup: %u  ", LocalActiveCup+1);
			for (uint16_t J = 0; J < ANALOG_MAX_CHANNELS; J++) {
				uint32_t Accumulator0 = 0;
				uint32_t Accumulator1 = 0;
				for (uint16_t K = 0; K < ADC_RAW_BUFFER_SIZE; K++) {
					Accumulator0 += RawBufferAdc0[J][K];
					Accumulator1 += RawBufferAdc1[J][K];
				}
				printf("Ch%u: %4lu  %4lu  ", J, Accumulator0, Accumulator1);
			}
			printf("\r\n");
		}
	}


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