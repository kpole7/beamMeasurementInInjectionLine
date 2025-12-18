/// @file mainTimer.c

#include "pico/stdlib.h"

#include "adcInputs.h"
#include "mainTimer.h"

//---------------------------------------------------------------------------------------------------
// Macro directives
//---------------------------------------------------------------------------------------------------

#define TIMER_INTERRUPT_INTERVAL_US	10000

#define TIME_DIVIDER_ADC			25



//---------------------------------------------------------------------------------------------------
// Local variables
//---------------------------------------------------------------------------------------------------

static repeating_timer_t CyclicTimer;

//---------------------------------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------------------------------

static bool repeatingTimerISR(repeating_timer_t *rt);

//---------------------------------------------------------------------------------------------------
// Function definitions
//---------------------------------------------------------------------------------------------------

/// @brief This function initializes the timer interrupt
void startPeriodicInterrupt(void){
    add_repeating_timer_us(
        -10000,                 // minus = absolute time
		repeatingTimerISR,
        NULL,
        &CyclicTimer
    );
}

/// @brief This is timer interrupt handler (ISR) for slow cyclic events
/// @callgraph
/// @callergraph
static bool repeatingTimerISR(repeating_timer_t *rt){
//	changeDebugPin1(true);

	// Analog-to-digital converter operation
	static uint8_t TimeCounterAdc;
	TimeCounterAdc++;
	if (TimeCounterAdc >= TIME_DIVIDER_ADC){
		TimeCounterAdc = 0;
		getVoltageSamples();
	}

	// continue cyclic interrupts
	return true;
}

