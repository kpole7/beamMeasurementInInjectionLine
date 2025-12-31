/// @file adcInputs.h
/// @brief This module measures input voltages on ADC0 and ADC1

#ifndef SOURCE_ADC_INPUTS_H_
#define SOURCE_ADC_INPUTS_H_

#include "pico/stdlib.h"

//---------------------------------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------------------------------

void initializeAdcMeasurements(void);

void getVoltageSamples(void);

float getVoltage(void);

#endif // SOURCE_ADC_INPUTS_H_
