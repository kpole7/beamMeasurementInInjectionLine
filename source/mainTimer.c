/// @file mainTimer.c

#include "pico/stdlib.h"

#include "analogInputs.h"
#include "debuggingTools.h"
#include "mainTimer.h"

//---------------------------------------------------------------------------------------------------
// Macro directives
//---------------------------------------------------------------------------------------------------

/// This constant defines the timer interrupt interval in microseconds. 
/// The negative value means that the time is absolute.
#define TIMER_INTERRUPT_INTERVAL_US (-1000)

//---------------------------------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------------------------------

/// @brief This flag synchronizes some events in the main loop. 
/// It is used in the timer ISR and in the main loop
atomic_bool TwoMillisecondsTimeTick;

/// @brief This flag synchronizes some events in the main loop. 
/// It is used in the timer ISR and in the main loop
atomic_bool SlowProcessesTimeTick1;

atomic_bool SlowProcessesTimeTick2;

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

/// This function initializes the timer interrupt
void startPeriodicInterrupt(void) {
	add_repeating_timer_us(TIMER_INTERRUPT_INTERVAL_US, // negative value -> absolute time
	                       repeatingTimerISR, NULL, &CyclicTimer);
}

/// @brief This is timer interrupt handler (ISR) for slow cyclic events
/// The interrupt frequency is defined by TIMER_INTERRUPT_INTERVAL_US.
/// @callgraph
/// @callergraph
static bool repeatingTimerISR(repeating_timer_t *rt) {
	static uint8_t TimeDivider;
	TimeDivider++;
	TimeDivider &= 7;

	if (0 == TimeDivider) {
		// frequency = 1000Hz / 8 = 125Hz (measured on 2026.06.30)
		atomic_store_explicit(&SlowProcessesTimeTick1, true, memory_order_release);
	}
	if (4 == TimeDivider) {
		atomic_store_explicit(&SlowProcessesTimeTick2, true, memory_order_release);
	}

	if (1 == (TimeDivider & 1)) {
		// frequency = 1000Hz / 2 = 500Hz
		atomic_store_explicit(&TwoMillisecondsTimeTick, true, memory_order_release);
	}
	// continue cyclic interrupts
	return true;
}
