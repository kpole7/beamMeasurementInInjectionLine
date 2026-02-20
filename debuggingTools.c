/// @file debuggingTools.c
// This source code file was written by K.O. (2025 - 2026)

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "modbusConfig.h"
#include "masterConfig.h"
#include "debuggingTools.h"


// simulation of signal propagation from user request to insert/remove cup to feedback from limit switch
atomic_uint_fast16_t DebugCountdownPropagationFromCoilToSwitch1;
atomic_uint_fast16_t DebugCountdownPropagationFromCoilToSwitch2;
atomic_uint_fast16_t DebugCountdownPropagationFromCoilToSwitch3;
atomic_bool DebugCompletedPropagationFromCoilToSwitch1;
atomic_bool DebugCompletedPropagationFromCoilToSwitch2;
atomic_bool DebugCompletedPropagationFromCoilToSwitch3;


#if MODBUS_DEBUG_MODE

//..............................................................................
// Local variables
//..............................................................................

static uint64_t AuxiliaryPrintTime;
static char AuxiliaryPrintOldCharacter;

static LogEventType LogTable[LOG_SIZE];
static volatile uint16_t LogPrintLast;
static atomic_uint_fast16_t LogIndex;

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
	uint16_t TemporaryIndex = atomic_load_explicit( &LogIndex, memory_order_acquire );

	if (TemporaryIndex >= LOG_SIZE){
		return;
	}
	LogTable[TemporaryIndex].time = (uint32_t)time_us_64();
	strncpy(LogTable[TemporaryIndex].name,Name,LOG_EVENT_NAME_SIZE);
	LogTable[TemporaryIndex].name[LOG_EVENT_NAME_SIZE-1] = 0;
	LogTable[TemporaryIndex].value = NewValue;

	if (atomic_load_explicit( &LogIndex, memory_order_acquire ) == TemporaryIndex){
		atomic_store_explicit( &LogIndex, TemporaryIndex+1, memory_order_release );
	}
}

// This function is similar to logAddEvent, but only stores a record only if the Name
// is different from the previous one.
void logOnceAddEvent(const char* Name,uint16_t NewValue){
	if(atomic_load_explicit( &LogIndex, memory_order_acquire ) > 0){
		if(0 == strncmp( LogTable[atomic_load_explicit( &LogIndex, memory_order_acquire )-1].name,
				Name, LOG_EVENT_NAME_SIZE ))
		{
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
	if (0 == LogPrintLast){
		printf("\r\n\r\nLog\r\n-------------------------\r\n");
	}
	else{
		printf("\r\n");
	}
	uint16_t TemporaryIndex = atomic_load_explicit( &LogIndex, memory_order_acquire );
	for( P=LogPrintLast; P < TemporaryIndex; P++){
		Second = (double)LogTable[P].time;
		Second *= 1e-6;
		if(LogTable[P].value != 0xFFFFu){
			printf("%4u %9.6f %s (%u)\r\n",P,Second,LogTable[P].name,LogTable[P].value);
		}
		else{
			printf("%4u %9.6f %s\r\n",P,Second,LogTable[P].name);
		}
	}
	LogPrintLast = TemporaryIndex;
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

#endif // MODBUS_DEBUG_MODE

void initInputPortJP2(void){
	gpio_init(AUXILIARY_INPUT_JP2);
	gpio_set_dir(AUXILIARY_INPUT_JP2, GPIO_IN);
	gpio_pull_up(AUXILIARY_INPUT_JP2);
}

// This function checks if the jumper JP1 is present.
bool readInputPortJP2(void){
	return(gpio_get(AUXILIARY_INPUT_JP2));
}

void auxiliaryOutputsInitialize(void){
	gpio_init(AUXILIARY_PIN_1);
	gpio_set_dir(AUXILIARY_PIN_1, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_1, false);

	gpio_init(AUXILIARY_PIN_2);
	gpio_set_dir(AUXILIARY_PIN_2, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_2, false);
}

void auxiliaryPinOutputValue1(bool Value){
	gpio_put(AUXILIARY_PIN_1, Value);
}

void auxiliaryPinOutputValue2(bool Value){
	gpio_put(AUXILIARY_PIN_2, Value);
}

/// @brief This function reads text commands entered in the terminal and executes them.
/// @param RegistersToBeChangedPtr pointer to a uint16_t variable (for example, a Modbus register) that is to be modified
/// @param RegistersToBeChangedNumber number of elements in the table pointed to by RegistersToBeChangedPtr
/// @param FirstName single-letter name used in a command to modify the first element
/// The commands have the form "A=num;" where num is a decimal or hexadecimal number.
/// Exemplary commands:
/// A=123;      means RegistersToBeChangedPtr[0] = 123
/// B=0xA5;		means RegistersToBeChangedPtr[1] = 0xA5
void debugTerminalCommandInterpreter(uint16_t * RegistersToBeChangedPtr, uint16_t RegistersToBeChangedNumber, char FirstName){
	static char Buffer[103];
	static uint32_t Index, Tics;
	int InputCharacter = getchar_timeout_us(0);  // non-blocking read
	if (InputCharacter != PICO_ERROR_TIMEOUT) {
		Tics = 0;
		if (Index < sizeof(Buffer)-3){
			Buffer[Index] = InputCharacter;
			Index++;
			if (';' == InputCharacter){
				Buffer[Index] = 0;

				uint32_t Argument;
				char * EndPtr;
				char * SemicolonPtr = strchr(Buffer, ';');
				if (SemicolonPtr[1] != 0){ // check if the first semicolon is the only semicolon and if it is the last character
					SemicolonPtr = NULL;
				}
				if ((FirstName <= Buffer[0]) && (FirstName+(char)RegistersToBeChangedNumber > Buffer[0]) && ('=' == Buffer[1]) && (strlen(Buffer) >= 4) && (SemicolonPtr != NULL)){
					if ((strstr(Buffer, "=0x") == Buffer+1) && (strlen(Buffer) >= 6)){
						Argument = (uint32_t)strtoul( Buffer+4, &EndPtr, 16 );
					}
					else{
						Argument = (uint32_t)strtoul( Buffer+2, &EndPtr, 10 );
					}
					if ((Argument < 0x10000) && (';' == EndPtr[0])){
						printf("Command %c=%d=0x%04X\r\n", Buffer[0], Argument, Argument );
					}
					else{
						printf("Wrong command <%s> \r\n", Buffer );
					}
				}
				else{
					printf("Unknown command <%s>\r\n", Buffer );
				}

				uint8_t J = Buffer[0] - FirstName;
				if (J < RegistersToBeChangedNumber){
					RegistersToBeChangedPtr[J] = Argument;
				}

				Index = 0;
			}
		}
	}
	else{
		if (Tics < 10){
			Tics++;
		}
		else{
			if (0 != Index){
				Index = 0; // timeout => clear the buffer
				printf("Wyczyszczono bufor\r\n" );
			}
		}
	}
}






