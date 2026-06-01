/// @file logicInputs.h
// This header file was written by K.O. (2026)

#ifndef SOURCE_LOGIC_INPUTS_H_
#define SOURCE_LOGIC_INPUTS_H_


#define LIMIT_SWITCH_1_INDEX 0
#define LIMIT_SWITCH_2_INDEX 1
#define LIMIT_SWITCH_3A_INDEX 2
#define LIMIT_SWITCH_3B_INDEX 3
#define EXTERNAL_INHIBITION_INDEX 4


void initializeLogicInputs(void);

void logicInputsTick(void);

#endif // SOURCE_LOGIC_INPUTS_H_