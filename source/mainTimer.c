/// @file mainTimer.c

#include "pico/stdlib.h"

#include "adcInputs.h"
#include "debuggingTools.h"
#include "mainTimer.h"

//---------------------------------------------------------------------------------------------------
// Macro directives
//---------------------------------------------------------------------------------------------------

#define TIMER_INTERRUPT_INTERVAL_US (-1000) // minus = absolute time

//---------------------------------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------------------------------

/// @brief This flag is used in the timer ISR and in the main loop
atomic_bool TwoMillisecondsTimeTick;

/// @brief This flag is used in the timer ISR and in the main loop
atomic_bool SixtyFourMillisecondsTimeTick;

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
void startPeriodicInterrupt(void) {
	add_repeating_timer_us(TIMER_INTERRUPT_INTERVAL_US, // negative value -> absolute time
	                       repeatingTimerISR, NULL, &CyclicTimer);
}

/// @brief This is timer interrupt handler (ISR) for slow cyclic events
/// The interrupt frequency is 1000 Hz.
/// @callgraph
/// @callergraph
static bool repeatingTimerISR(repeating_timer_t *rt) {
	static uint8_t TimeDivider;

	auxiliaryPinOutputValue1(false);
	auxiliaryPinOutputValue2(true);

	TimeDivider++;
	TimeDivider &= 63;

	if (0 == TimeDivider) {
		// frequency = 1000Hz / 64 = 15.625Hz
		auxiliaryPinOutputValue1(true);

		atomic_store_explicit(&SixtyFourMillisecondsTimeTick, true, memory_order_release);
	}

	if (4 == (TimeDivider & 7)) {
		// frequency = 1000Hz / 8 = 125Hz
		getVoltageSamples();

		// simulation of signal propagation from user request to insert/remove cup to feedback from limit switch
		if (atomic_load_explicit(&DebugCountdownPropagationFromCoilToSwitch1, memory_order_acquire) > 0) {
			if (atomic_load_explicit(&DebugCountdownPropagationFromCoilToSwitch1, memory_order_acquire) == 1) {
				atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch1, true, memory_order_release);
			}
			atomic_fetch_add_explicit(&DebugCountdownPropagationFromCoilToSwitch1, -1, memory_order_acq_rel);
		}
		if (atomic_load_explicit(&DebugCountdownPropagationFromCoilToSwitch2, memory_order_acquire) > 0) {
			if (atomic_load_explicit(&DebugCountdownPropagationFromCoilToSwitch2, memory_order_acquire) == 1) {
				atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch2, true, memory_order_release);
			}
			atomic_fetch_add_explicit(&DebugCountdownPropagationFromCoilToSwitch2, -1, memory_order_acq_rel);
		}
		if (atomic_load_explicit(&DebugCountdownPropagationFromCoilToSwitch3, memory_order_acquire) > 0) {
			if (atomic_load_explicit(&DebugCountdownPropagationFromCoilToSwitch3, memory_order_acquire) == 1) {
				atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch3, true, memory_order_release);
			}
			atomic_fetch_add_explicit(&DebugCountdownPropagationFromCoilToSwitch3, -1, memory_order_acq_rel);
		}
	}

	if (1 == (TimeDivider & 1)) {
		// frequency = 1000Hz / 2 = 500Hz
		atomic_store_explicit(&TwoMillisecondsTimeTick, true, memory_order_release);
	}

	auxiliaryPinOutputValue2(false);

	// continue cyclic interrupts
	return true;
}
