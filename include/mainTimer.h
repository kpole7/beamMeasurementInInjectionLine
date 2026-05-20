/// @file mainTimer.h
/// @brief This module implements clock timing for real-time processes

#ifndef SOURCE_MAIN_TIMER_H_
#define SOURCE_MAIN_TIMER_H_

#include "pico/stdlib.h"
#include <stdatomic.h>
#include <stdbool.h>

//---------------------------------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------------------------------

extern atomic_bool TwoMillisecondsTimeTick;

extern atomic_bool SlowProcessesTimeTick1, SlowProcessesTimeTick2;

//---------------------------------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------------------------------

void startPeriodicInterrupt(void);

#endif /* SOURCE_MAIN_TIMER_H_ */
