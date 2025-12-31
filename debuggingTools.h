/// @file debuggingTools.h
// This header file was written by K.O. (2025 - 2026)

#ifndef _DEBUGGING_TOOLS_H
#define _DEBUGGING_TOOLS_H

#include "masterConfig.h"

#define AUXILIARY_INPUT_JP1		14	// The jumper JP1 tells pico whether printing should be enabled

#define AUXILIARY_INPUT_JP2		10

#define AUXILIARY_PIN_1			18
#define AUXILIARY_PIN_2			19


#if MODBUS_DEBUG_MODE

// Debugging of Modbus state machine.
// Size of the table (log) of events.
#define LOG_SIZE				1024

#define LOG_EVENT_NAME_SIZE		10

// Debugging of Modbus state machine.
// This structure defines a data record describing an event
typedef struct{
	uint32_t time;
	char name[LOG_EVENT_NAME_SIZE];
	uint16_t value;
}LogEventType;

extern bool IsChangeModbusWrite;

void initAuxiliaryPrintouts(void);

void auxiliaryPrintCharacter(char C);
void auxiliaryPrintOncePerSecondCharacter(char C);
void auxiliaryPrintString(const char*CPtr);
void auxiliaryPrintHexBytes(const uint8_t * DataPtr, uint8_t Count);
void auxiliaryPrintUInt16(uint16_t Data);

void logAddEvent(const char* Name,uint16_t NewValue);
void logOnceAddEvent(const char* Name,uint16_t NewValue);
void logPrintAll(uint16_t WaitMilliseconds);
void logPrintNew(uint16_t WaitMilliseconds);

// This function initializes IO port connected to jumper JP1.
void initInputPortJP1(void);

// This function checks if the jumper JP1 is present.
bool readInputPortJP1(void);

void initInputPortJP2(void);

#endif // MODBUS_DEBUG_MODE

bool readInputPortJP2(void);

void auxiliaryOutputsInitialize(void);
void auxiliaryPinOutputValue1(bool Value);
void auxiliaryPinOutputValue2(bool Value);

#endif // _DEBUGGING_TOOLS_H
