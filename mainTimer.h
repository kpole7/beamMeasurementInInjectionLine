/// @file mainTimer.h
/// @brief This module implements clock timing for real-time processes

#ifndef SOURCE_MAIN_TIMER_H_
#define SOURCE_MAIN_TIMER_H_

#include <stdbool.h>
#include <stdatomic.h>
#include "pico/stdlib.h"

//---------------------------------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------------------------------

extern atomic_bool TwoMillisecondsTimeTick;

extern atomic_bool SixtyFourMillisecondsTimeTick;

//---------------------------------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------------------------------

/// @brief This function initializes the timer interrupt
void startPeriodicInterrupt(void);

#endif /* SOURCE_MAIN_TIMER_H_ */
