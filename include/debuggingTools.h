/// @file debuggingTools.h
// This header file was written by K.O. (2025 - 2026)

#ifndef DEBUGGING_TOOLS_H
#define DEBUGGING_TOOLS_H

#include "masterConfig.h"
#include <stdatomic.h>

#define AUXILIARY_INPUT_JP1 28

#define AUXILIARY_PIN_1 8
#define AUXILIARY_PIN_2 22
#define AUXILIARY_PIN_3 14
#define AUXILIARY_PIN_4 15

#define PRINTOUTS_ANALOG 0x0001u
#define PRINTOUTS_LOGIC 0x0002u
#define PRINTOUTS_ACTUATORS 0x0004u
#define PRINTOUTS_SIMULATION 0x0008u
#define PRINTOUTS_SIM_EVENT 0x0010u
#define ENABLE_SIMULATION 0x8000u

#define SIM_EVENT_SWITCH1_PERMANENT_OFF 1u
#define SIM_EVENT_SWITCH1_PERMANENT_ON 2u
#define SIM_EVENT_SWITCH2_PERMANENT_OFF 3u
#define SIM_EVENT_SWITCH2_PERMANENT_ON 4u
#define SIM_EVENT_SWITCH3A_PERMANENT_OFF 5u
#define SIM_EVENT_SWITCH3A_PERMANENT_ON 6u
#define SIM_EVENT_SWITCH3B_PERMANENT_OFF 7u
#define SIM_EVENT_SWITCH3B_PERMANENT_ON 8u
#define SIM_EVENT_SWITCH1_TEMPORARY_OFF 9u
#define SIM_EVENT_SWITCH1_TEMPORARY_ON 10u
#define SIM_EVENT_SWITCH2_TEMPORARY_OFF 11u
#define SIM_EVENT_SWITCH2_TEMPORARY_ON 12u
#define SIM_EVENT_SWITCH3A_TEMPORARY_OFF 13u
#define SIM_EVENT_SWITCH3A_TEMPORARY_ON 14u
#define SIM_EVENT_SWITCH3B_TEMPORARY_OFF 15u
#define SIM_EVENT_SWITCH3B_TEMPORARY_ON 16u


#if defined(APP_DEBUG_BUILD)
#define APP_DEBUG 1
#else
#define APP_DEBUG 0
#endif

void initializeDebuggingTools(void);
void auxiliaryPinOutputValue1(bool Value);
void auxiliaryPinOutputValue2(bool Value);

void initializeTimeStamp(void);
void updateTimeStamp( uint16_t MillisecondsToAdd );
char *getTimeStampString(void);
char *getTimeStampStringWithoutUpdate(void);

void printChangedRegisters( const char *ContextComment );

void debugMainLoopTick(void);

#if DEBUG_SIMULATION_MODE
void simulationMainLoopTick(void);
bool simulateInput(int InputIndex);
#endif // DEBUG_SIMULATION_MODE

// simulation of signal propagation from user request to insert/remove cup to feedback from limit switch
extern atomic_uint_fast16_t DebugCountdownPropagationFromCoilToSwitch1;
extern atomic_uint_fast16_t DebugCountdownPropagationFromCoilToSwitch2;
extern atomic_uint_fast16_t DebugCountdownPropagationFromCoilToSwitch3;
extern atomic_bool DebugCompletedPropagationFromCoilToSwitch1;
extern atomic_bool DebugCompletedPropagationFromCoilToSwitch2;
extern atomic_bool DebugCompletedPropagationFromCoilToSwitch3;

#endif // DEBUGGING_TOOLS_H
