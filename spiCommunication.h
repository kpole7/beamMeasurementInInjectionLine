// This header file was written by K.O. (2025 - 2026)

#ifndef _SPI_COMMUNICATION_H
#define _SPI_COMMUNICATION_H

#include <stdbool.h>

// Initialization of hardware peripherals and initialization of variables that
// are used to communicate with the power source via SPI.
void initializeSPI(void);

// This function is to be cyclically called to communicate via SPI with the power source.
// This function stores current and voltage samples but does not calculate their statistical values.
// The function handles special situations described in the documentation (such as appearance of
// errors in the PSU)
//bool communicateHighCurrentSource(bool IsWriting);

// The function calculates statistical values of current and voltage based on samples
// stored in memory.
//void calculateStatistics(void);

#endif // _SPI_COMMUNICATION_H
