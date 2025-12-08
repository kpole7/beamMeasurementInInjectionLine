// This header file was written by K.O. (2024 - 2025)

#ifndef _DEBUGGING_TOOLS_H
#define _DEBUGGING_TOOLS_H

#include "masterConfig.h"

#define AUXILIARY_INPUT_JP1		14	// The jumper JP1 tells pico whether printing should be enabled
#define AUXILIARY_INPUT_JP2		13
#define AUXILIARY_INPUT_JP3		10
#define AUXILIARY_INPUT_JP4		9

#if AUXILIARY_OUTPUT_PINS
#define AUXILIARY_PIN_1			9
#define AUXILIARY_PIN_2			8
#define AUXILIARY_PIN_3			7
#endif


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
bool readInputPortJP2(void);

#endif // MODBUS_DEBUG_MODE

#if AUXILIARY_OUTPUT_PINS
void auxiliaryOutputsInitialize(void);
void auxiliaryPinOutputValue1(bool Value);
void auxiliaryPinOutputValue2(bool Value);
void auxiliaryPinOutputValue3(bool Value);
#endif

#if WRITING_ERROR_TEST
void initInputPortJP3(void);
bool readInputPortJP3(void);
#endif

#if SPI_SIMULATION_JP4
void initInputPortJP4(void);
bool readInputPortJP4(void);
#endif

#endif // _DEBUGGING_TOOLS_H
