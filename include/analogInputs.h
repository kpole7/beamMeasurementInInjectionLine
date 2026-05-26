/// @file analogInputs.h
/// @brief This module measures input voltages on ADC0 and ADC1

#ifndef SOURCE_ANALOG_INPUTS_H_
#define SOURCE_ANALOG_INPUTS_H_

#include <stdint.h>

//---------------------------------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------------------------------

void initializeAdcMeasurements(void);

void getVoltageSamples(void);

float getVoltage( uint16_t ChannelNumber );

#endif // SOURCE_ANALOG_INPUTS_H_
