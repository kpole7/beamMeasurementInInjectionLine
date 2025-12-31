/// @file adcInputs.c

#include <stdbool.h>
#include <stdatomic.h>
#include "pico/stdlib.h"
#include "adcInputs.h"
#include <math.h>
#include "hardware/adc.h"

//---------------------------------------------------------------------------------------------------
// Macro directives
//---------------------------------------------------------------------------------------------------

#define ADC_RAW_BUFFER_SIZE 	4
#define GPIO_FOR_ADC0			26

//---------------------------------------------------------------------------------------------------
// Local constants
//---------------------------------------------------------------------------------------------------

static const float GetVoltageCoefficient = 1.0 / ((float)ADC_RAW_BUFFER_SIZE);
static const float GetVoltageOffset = 0.0;

//---------------------------------------------------------------------------------------------------
// Local variables
//---------------------------------------------------------------------------------------------------

/// @brief This variable is used in timer interrupt handler
static atomic_uint_fast16_t RawBufferAdc0[ADC_RAW_BUFFER_SIZE];

/// @brief This variable is used in timer interrupt handler
/// Index for writing new samples from ADC0 and ADC1
static volatile uint32_t AdcBuffersHead = 0;

//---------------------------------------------------------------------------------------------------
// Function definitions
//---------------------------------------------------------------------------------------------------

/// @brief This function initializes peripherals for ADC measuring and the state machine for measurements
void initializeAdcMeasurements(void){
	AdcBuffersHead = 0;
	adc_init();
	adc_gpio_init(GPIO_FOR_ADC0);
}

/// @brief This function collects measurements from ADC; it is called only by the timer ISR (repeatingTimerISR)
void getVoltageSamples(void){

	// Measure ADC0
    adc_select_input(0);
    (void)adc_read();                // dummy read
    atomic_store_explicit( &RawBufferAdc0[AdcBuffersHead], adc_read(), memory_order_release );

    AdcBuffersHead++;
    if (AdcBuffersHead >= ADC_RAW_BUFFER_SIZE){
    	AdcBuffersHead = 0;
    }
}

/// @brief This function measures the voltage at ADC input and make some calculations
/// The function acts in the main loop
float getVoltage(void){
	uint32_t Accumulator = 0;
	for (uint8_t J = 0; J < ADC_RAW_BUFFER_SIZE; J++){
		Accumulator += atomic_load_explicit( &RawBufferAdc0[J], memory_order_acquire )                    ;
	}
	return (float)Accumulator * GetVoltageCoefficient - GetVoltageOffset;
}

