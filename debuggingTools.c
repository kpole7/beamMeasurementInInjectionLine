// This source code file was written by K.O. (2024 - 2025)

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "modbusConfig.h"
#include "masterConfig.h"
#include "debuggingTools.h"

#if MODBUS_DEBUG_MODE

//..............................................................................
// Local variables
//..............................................................................

static uint64_t AuxiliaryPrintTime;
static char AuxiliaryPrintOldCharacter;

static LogEventType LogTable[LOG_SIZE];
static volatile uint16_t LogIndex, LogPrintLast;


//..............................................................................
// Definitions of interface functions
//..............................................................................


void initAuxiliaryPrintouts(void){
	AuxiliaryPrintOldCharacter = 0;
    AuxiliaryPrintTime = 0;
	LogPrintLast = 0;
}

void auxiliaryPrintCharacter(char C){
	printf("%c",C);
}

void auxiliaryPrintOncePerSecondCharacter(char C){
	uint64_t AuxiliaryTime;
	if(AuxiliaryPrintOldCharacter == C){
		AuxiliaryTime = time_us_64();
		if (AuxiliaryTime < AuxiliaryPrintTime + 1000000ul){
			return;
		}
		AuxiliaryPrintTime = AuxiliaryTime;
	}
	else{
		AuxiliaryPrintOldCharacter = C;
	}
	printf("%c",C);
}

void auxiliaryPrintString(const char*CPtr){
	printf(CPtr);
}

void auxiliaryPrintHexBytes(const uint8_t * DataPtr, uint8_t Count){
	uint8_t M;
	if (0 == Count){
		return;
	}
	printf("%02X", DataPtr[0]);
	for (M=1; M<Count; M++){
		printf(" %02X", DataPtr[M]);
	}
}

void auxiliaryPrintUInt16(uint16_t Data){
	printf("%u",Data);
}

// This function stores one record for a given checkpoint.
// The record is of type LogEventType.
// All records are stored in LogTable.
// When the LogTable is full, no new record can be saved.
// This function is in the main loop in the UART interrupt handler and in the timer interrupt handler.
void logAddEvent(const char* Name,uint16_t NewValue){
	if (LogIndex >= LOG_SIZE){
		return;
	}
	LogTable[LogIndex].time = (uint32_t)time_us_64();
	strncpy(LogTable[LogIndex].name,Name,LOG_EVENT_NAME_SIZE);
	LogTable[LogIndex].name[LOG_EVENT_NAME_SIZE-1] = 0;
	LogTable[LogIndex].value = NewValue;
	LogIndex++;
}

// This function is similar to logAddEvent, but only stores a record only if the Name
// is different from the previous one.
void logOnceAddEvent(const char* Name,uint16_t NewValue){
	if(LogIndex>0){
		if(0==strncmp(LogTable[LogIndex-1].name,Name,LOG_EVENT_NAME_SIZE)){
			return;
		}
	}
	logAddEvent(Name,NewValue);
}

void logPrintAll(uint16_t WaitMilliseconds){
	LogPrintLast=0;
	logPrintNew(0);
	printf("=========================\r\n");
	if(WaitMilliseconds > 0){
		sleep_ms(WaitMilliseconds);
	}
}

void logPrintNew(uint16_t WaitMilliseconds){
	double Second;
	uint16_t P;
	if (0==LogPrintLast){
		printf("\r\n\r\nLog\r\n-------------------------\r\n");
	}
	else{
		printf("\r\n");
	}
	for(P=LogPrintLast;P<LogIndex;P++){
		Second = (double)LogTable[P].time;
		Second *= 1e-6;
		if(LogTable[P].value != 0xFFFFu){
			printf("%4u %9.6f %s (%u)\r\n",P,Second,LogTable[P].name,LogTable[P].value);
		}
		else{
			printf("%4u %9.6f %s\r\n",P,Second,LogTable[P].name);
		}
	}
	LogPrintLast = LogIndex;
	if(WaitMilliseconds > 0){
		sleep_ms(WaitMilliseconds);
	}
}

// This function initializes IO port connected to jumper JP1.
void initInputPortJP1(void){
	gpio_init(AUXILIARY_INPUT_JP1);
	gpio_set_dir(AUXILIARY_INPUT_JP1, GPIO_IN);
	gpio_pull_up(AUXILIARY_INPUT_JP1);
}

// This function checks if the jumper JP1 is present.
bool readInputPortJP1(void){
	return(gpio_get(AUXILIARY_INPUT_JP1));
}

void initInputPortJP2(void){
	gpio_init(AUXILIARY_INPUT_JP2);
	gpio_set_dir(AUXILIARY_INPUT_JP2, GPIO_IN);
	gpio_pull_up(AUXILIARY_INPUT_JP2);
}

bool readInputPortJP2(void){
	return(gpio_get(AUXILIARY_INPUT_JP2));
}
#endif // MODBUS_DEBUG_MODE

#if WRITING_ERROR_TEST
void initInputPortJP3(void){
	gpio_init(AUXILIARY_INPUT_JP3);
	gpio_set_dir(AUXILIARY_INPUT_JP3, GPIO_IN);
	gpio_pull_up(AUXILIARY_INPUT_JP3);
}

bool readInputPortJP3(void){
	return(gpio_get(AUXILIARY_INPUT_JP3));
}
#endif

#if SPI_SIMULATION_JP4
void initInputPortJP4(void){
	gpio_init(AUXILIARY_INPUT_JP4);
	gpio_set_dir(AUXILIARY_INPUT_JP4, GPIO_IN);
	gpio_pull_up(AUXILIARY_INPUT_JP4);
}

bool readInputPortJP4(void){
	return(gpio_get(AUXILIARY_INPUT_JP4));
}
#endif

#if AUXILIARY_OUTPUT_PINS
void auxiliaryOutputsInitialize(void){
	gpio_init(AUXILIARY_PIN_1);
	gpio_set_dir(AUXILIARY_PIN_1, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_1, false);

	gpio_init(AUXILIARY_PIN_2);
	gpio_set_dir(AUXILIARY_PIN_2, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_2, false);

	gpio_init(AUXILIARY_PIN_3);
	gpio_set_dir(AUXILIARY_PIN_3, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_3, false);
}

void auxiliaryPinOutputValue1(bool Value){
	gpio_put(AUXILIARY_PIN_1, Value);
}

void auxiliaryPinOutputValue2(bool Value){
	gpio_put(AUXILIARY_PIN_2, Value);
}

void auxiliaryPinOutputValue3(bool Value){
	gpio_put(AUXILIARY_PIN_3, Value);
}
#endif
