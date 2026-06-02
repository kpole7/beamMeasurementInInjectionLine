/// @file debuggingTools.c
// This source code file was written by K.O. (2025 - 2026)

#include "debuggingTools.h"
#include "compilationTime.h"
#include "masterConfig.h"
#include "modbusConfig.h"
#include "logicInputs.h"
#include "sharedData.h"
#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAIN_LOOP_TICK_PERIOD_MS 2u
#define SIMULATION_1_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON		(1000ul / MAIN_LOOP_TICK_PERIOD_MS) // 1 s
#define SIMULATION_1_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF		(200ul / MAIN_LOOP_TICK_PERIOD_MS)  // 200 ms
#define SIMULATION_2_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON		(1000ul / MAIN_LOOP_TICK_PERIOD_MS) // 1 s
#define SIMULATION_2_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF		(200ul / MAIN_LOOP_TICK_PERIOD_MS)  // 200 ms

#define SIMULATION_3_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_B_OFF		(100ul / MAIN_LOOP_TICK_PERIOD_MS)  // 100 ms
#define SIMULATION_3_PROPAGATION_FROM_SWITCH_B_OFF_TO_SWITCH_A_ON	(7000ul / MAIN_LOOP_TICK_PERIOD_MS) // 7 s
#define SIMULATION_3_PROPAGATION_FROM_INSERTED_TO_SWITCH_A_OFF		(200ul / MAIN_LOOP_TICK_PERIOD_MS)  // 200 ms
#define SIMULATION_3_PROPAGATION_FROM_SWITCH_A_OFF_TO_SWITCH_B_ON	(9000ul / MAIN_LOOP_TICK_PERIOD_MS) // 9 s

#define SIMULATION_STATE_1_REST_OUTSIDE 0
#define SIMULATION_STATE_1_GOING_INSIDE 1
#define SIMULATION_STATE_1_REST_INSIDE 2
#define SIMULATION_STATE_1_GOING_OUTSIDE 3

#define SIMULATION_STATE_2_REST_OUTSIDE 0
#define SIMULATION_STATE_2_GOING_INSIDE 1
#define SIMULATION_STATE_2_REST_INSIDE 2
#define SIMULATION_STATE_2_GOING_OUTSIDE 3

#define SIMULATION_STATE_3_REST_OUTSIDE 0
#define SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_B 1
#define SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_A 2
#define SIMULATION_STATE_3_REST_INSIDE 3
#define SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_A 4
#define SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_B 5
#define SIMULATION_STATE_3_REST_IN_THE_MIDDLE 6

#define SWITCH_PRESSED false
#define SWITCH_RELEASED true

//..............................................................................
// Local variables
//..............................................................................

#if MODBUS_DEBUG_MODE
static uint64_t AuxiliaryPrintTime;
static char AuxiliaryPrintOldCharacter;

static LogEventType LogTable[LOG_SIZE];
static volatile uint16_t LogPrintLast;
static atomic_uint_fast16_t LogIndex;
#endif // MODBUS_DEBUG_MODE

#if DEBUG_SIMULATION_MODE

// This table is used in the debug simulation mode to simulate the state of logic inputs.
// Indexes are defined in logicInputs.h
bool SimulationInputs[5] = { true, true, true, true, true };

uint16_t SimulationState1;
uint16_t SimulationState2;
uint16_t SimulationState3;

uint16_t SimulationCounter1;
uint16_t SimulationCounter2;
uint16_t SimulationCounter3;

uint16_t SimulationClockMinutes;
uint16_t SimulationClockSeconds;
uint16_t SimulationClockMilliseconds;

#endif // DEBUG_SIMULATION_MODE


//..............................................................................
// Local function prototypes
//..............................................................................

static void commandInterpreterEssentials(uint16_t *RegistersToBeChangedPtr, uint16_t RegistersToBeChangedNumber, char FirstName, char *StaticBuffer);

//..............................................................................
// Definitions of interface functions
//..............................................................................

#if MODBUS_DEBUG_MODE

void initAuxiliaryPrintouts(void) {
	AuxiliaryPrintOldCharacter = 0;
	AuxiliaryPrintTime = 0;
	LogPrintLast = 0;
}

void auxiliaryPrintCharacter(char C) { printf("%c", C); }

void auxiliaryPrintOncePerSecondCharacter(char C) {
	uint64_t AuxiliaryTime;
	if (AuxiliaryPrintOldCharacter == C) {
		AuxiliaryTime = time_us_64();
		if (AuxiliaryTime < AuxiliaryPrintTime + 1000000ul) {
			return;
		}
		AuxiliaryPrintTime = AuxiliaryTime;
	}
	else {
		AuxiliaryPrintOldCharacter = C;
	}
	printf("%c", C);
}

void auxiliaryPrintString(const char *CPtr) { printf(CPtr); }

void auxiliaryPrintHexBytes(const uint8_t *DataPtr, uint8_t Count) {
	uint8_t M;
	if (0 == Count) {
		return;
	}
	printf("%02X", DataPtr[0]);
	for (M = 1; M < Count; M++) {
		printf(" %02X", DataPtr[M]);
	}
}

void auxiliaryPrintUInt16(uint16_t Data) { printf("%u", Data); }

// This function stores one record for a given checkpoint.
// The record is of type LogEventType.
// All records are stored in LogTable.
// When the LogTable is full, no new record can be saved.
// This function is in the main loop in the UART interrupt handler and in the timer interrupt handler.
void logAddEvent(const char *Name, uint16_t NewValue) {
	uint16_t TemporaryIndex = atomic_load_explicit(&LogIndex, memory_order_acquire);

	if (TemporaryIndex >= LOG_SIZE) {
		return;
	}
	LogTable[TemporaryIndex].time = (uint32_t)time_us_64();
	strncpy(LogTable[TemporaryIndex].name, Name, LOG_EVENT_NAME_SIZE);
	LogTable[TemporaryIndex].name[LOG_EVENT_NAME_SIZE - 1] = 0;
	LogTable[TemporaryIndex].value = NewValue;

	if (atomic_load_explicit(&LogIndex, memory_order_acquire) == TemporaryIndex) {
		atomic_store_explicit(&LogIndex, TemporaryIndex + 1, memory_order_release);
	}
}

// This function is similar to logAddEvent, but only stores a record only if the Name
// is different from the previous one.
void logOnceAddEvent(const char *Name, uint16_t NewValue) {
	if (atomic_load_explicit(&LogIndex, memory_order_acquire) > 0) {
		if (0 == strncmp(LogTable[atomic_load_explicit(&LogIndex, memory_order_acquire) - 1].name, Name, LOG_EVENT_NAME_SIZE)) {
			return;
		}
	}
	logAddEvent(Name, NewValue);
}

void logPrintAll(uint16_t WaitMilliseconds) {
	LogPrintLast = 0;
	logPrintNew(0);
	printf("=========================\r\n");
	if (WaitMilliseconds > 0) {
		sleep_ms(WaitMilliseconds);
	}
}

void logPrintNew(uint16_t WaitMilliseconds) {
	double Second;
	uint16_t P;
	if (0 == LogPrintLast) {
		printf("\r\n\r\nLog\r\n-------------------------\r\n");
	}
	else {
		printf("\r\n");
	}
	uint16_t TemporaryIndex = atomic_load_explicit(&LogIndex, memory_order_acquire);
	for (P = LogPrintLast; P < TemporaryIndex; P++) {
		Second = (double)LogTable[P].time;
		Second *= 1e-6;
		if (LogTable[P].value != 0xFFFFu) {
			printf("%4u %9.6f %s (%u)\r\n", P, Second, LogTable[P].name, LogTable[P].value);
		}
		else {
			printf("%4u %9.6f %s\r\n", P, Second, LogTable[P].name);
		}
	}
	LogPrintLast = TemporaryIndex;
	if (WaitMilliseconds > 0) {
		sleep_ms(WaitMilliseconds);
	}
}

// This function initializes IO port connected to jumper JP1.
void initInputPortJP1(void) {
	gpio_init(AUXILIARY_INPUT_JP1);
	gpio_set_dir(AUXILIARY_INPUT_JP1, GPIO_IN);
	gpio_pull_up(AUXILIARY_INPUT_JP1);
}

// This function checks if the jumper JP1 is present.
bool readInputPortJP1(void) { return (gpio_get(AUXILIARY_INPUT_JP1)); }

#endif // MODBUS_DEBUG_MODE

/// @brief This function initializes the auxiliary output pins for testing purposes.
void auxiliaryOutputsInitialize(void) {
	gpio_init(AUXILIARY_PIN_1);
	gpio_set_dir(AUXILIARY_PIN_1, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_1, false);

	gpio_init(AUXILIARY_PIN_2);
	gpio_set_dir(AUXILIARY_PIN_2, GPIO_OUT);
	gpio_put(AUXILIARY_PIN_2, false);
}

void auxiliaryPinOutputValue1(bool Value) { gpio_put(AUXILIARY_PIN_1, Value); }

void auxiliaryPinOutputValue2(bool Value) { gpio_put(AUXILIARY_PIN_2, Value); }

/// @brief This function reads text commands entered in the terminal and executes them.
/// The above-mentioned terminal is the terminal of the computer connected to the microcontroller via USB.
/// The Linux system uses usualy /dev/ttyACM0 serial port with the default settings (115200 baud, 8N1, hardware flow control). 
/// The commands are entered in the terminal and sent to the microcontroller when the user presses the Enter key.
/// The commands are used to modify the values of Modbus registers for testing purposes.
/// @param RegistersToBeChangedPtr pointer to a uint16_t variable (for example, a Modbus register) that is to be modified
/// @param RegistersToBeChangedNumber number of elements in the table pointed to by RegistersToBeChangedPtr
/// @param FirstName single-letter name used in a command to modify the first element
/// The commands have the form "A=num;" where num is a decimal or hexadecimal number.
/// Exemplary commands:
/// A=123;      means RegistersToBeChangedPtr[0] = 123
/// B=0xA5;		means RegistersToBeChangedPtr[1] = 0xA5
void debugTerminalCommandInterpreter(uint16_t *RegistersToBeChangedPtr, uint16_t RegistersToBeChangedNumber, char FirstName) {
	static char Buffer[101];
	static uint32_t Index;
	static uint32_t Tics;
	int InputCharacter = getchar_timeout_us(0); // non-blocking read
	if (InputCharacter != PICO_ERROR_TIMEOUT) {
		Tics = 0;
		if (Index < sizeof(Buffer) - 1) {
			Buffer[Index] = InputCharacter;
			Index++;
			if (';' == InputCharacter) {
				Buffer[Index] = 0;

				commandInterpreterEssentials(RegistersToBeChangedPtr, RegistersToBeChangedNumber, FirstName, Buffer);

				Index = 0;
			}
		}
	}
	else {
		if (Tics < 10) {
			Tics++;
		}
		else {
			if (0 != Index) {
				Index = 0; // timeout => clear the buffer
				printf("Wyczyszczono bufor\r\n");
			}
		}
	}
}

static void commandInterpreterEssentials(uint16_t *RegistersToBeChangedPtr, uint16_t RegistersToBeChangedNumber, char FirstName, char *StaticBuffer) {
	uint32_t Argument = 0;
	char *EndPtr;
	char *SemicolonPtr = strchr(StaticBuffer, ';');
	if (SemicolonPtr[1] != 0) { // check if the first semicolon is the only semicolon and if it is the last character
		SemicolonPtr = NULL;
	}
	if ((FirstName <= StaticBuffer[0]) && (FirstName + (char)RegistersToBeChangedNumber > StaticBuffer[0]) && ('=' == StaticBuffer[1]) &&
		(strlen(StaticBuffer) >= 4) && (SemicolonPtr != NULL)) {
		if ((strstr(StaticBuffer, "=0x") == StaticBuffer + 1) && (strlen(StaticBuffer) >= 6)) {
			Argument = (uint32_t)strtoul(StaticBuffer + 4, &EndPtr, 16);
		}
		else {
			Argument = (uint32_t)strtoul(StaticBuffer + 2, &EndPtr, 10);
		}
		if ((Argument < 0x10000) && (';' == EndPtr[0])) {
			printf("Command %c=%d=0x%04X\r\n", StaticBuffer[0], Argument, Argument);
		}
		else {
			printf("Wrong command <%s> \r\n", StaticBuffer);
		}
	}
	else {
		if (NULL != strstr(StaticBuffer, "ver;")) {
			printf("Time stamp=%s\r\n", CompilationTime);
		}
		else {
			printf("Unknown command <%s>\r\n", StaticBuffer);
		}
	}

	uint8_t J = StaticBuffer[0] - FirstName;
	if (J < RegistersToBeChangedNumber) {
		RegistersToBeChangedPtr[J] = Argument;
	}
}

#if DEBUG_SIMULATION_MODE

void initializeSimulation(void){
	SimulationClockMinutes = 0;
	SimulationClockSeconds = 0;
	SimulationClockMilliseconds = 0;
}

void simulationMainLoopTick(void){
	// 	Actuator 1 event simulation
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)]){
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)]) {
			if (SimulationState1 != SIMULATION_STATE_1_REST_OUTSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState1);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 1   ....|.........<<\r\n",
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState1 = SIMULATION_STATE_1_GOING_INSIDE;
				SimulationCounter1 = 0;
			}
		}
		else {
			if (SimulationState1 != SIMULATION_STATE_1_REST_INSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState1);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 1   >>..|.........\r\n",
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState1 = SIMULATION_STATE_1_GOING_OUTSIDE;
				SimulationCounter1 = 0;
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = false;
	}

	// 	Actuator 2 event simulation
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)]){
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)]) {
			if (SimulationState2 != SIMULATION_STATE_2_REST_OUTSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState2);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 2   ....|.........<<\r\n",
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState2 = SIMULATION_STATE_2_GOING_INSIDE;
				SimulationCounter2 = 0;
			}
		}
		else {
			if (SimulationState2 != SIMULATION_STATE_2_REST_INSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState2);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 2   >>..|.........\r\n",
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState2 = SIMULATION_STATE_2_GOING_OUTSIDE;
				SimulationCounter2 = 0;
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = false;
	}

	// 	Actuator 3 event simulation: insert
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)]){
		// Request to insert or to stop inserting the actuator 3
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)]) {
			// Request to insert
			if (SimulationState3 != SIMULATION_STATE_3_REST_OUTSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState3);
			}
			else if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)]) {
				printf("%03u:%02u:%02u  ERROR, Line %u, both control coils are on (conflict)\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, __LINE__);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 3   ....|...........|..<<\r\n",
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState3 = SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_B;
				SimulationCounter3 = 0;
			}
		}
		else {
			// Request to stop inserting
			if (SimulationState3 != SIMULATION_STATE_3_REST_INSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState3);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 3   []..|...........|..\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)] = false;
	}

	// 	Actuator 3 event simulation: extract
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)]){
		// Request to extract or to stop extracting the actuator 3
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)]) {
			// Request to extract
			if (SimulationState3 != SIMULATION_STATE_3_REST_OUTSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState3);
			}
			else if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)]) {
				printf("%03u:%02u:%02u  ERROR, Line %u, both control coils are on (conflict)\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, __LINE__);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 3   ....|...........|..<<\r\n",
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState3 = SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_B;
				SimulationCounter3 = 0;
			}
		}
		else {
			// Request to stop extracting
			if (SimulationState3 != SIMULATION_STATE_3_REST_INSIDE) {
				printf("%03u:%02u:%02u  Warning, Line %u, unexpected state %u\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds, 
					__LINE__, SimulationState3);
			}
			else{
				printf("%03u:%02u:%02u  Actuator 3   >>..|...........|..\r\n", 
					SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
				SimulationState3 = SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_A;
				SimulationCounter3 = 0;
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)] = false;
	}

	// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

	//  Actuator 1 time flow simulation
	if (SimulationState1 == SIMULATION_STATE_1_GOING_INSIDE) {
		SimulationCounter1++;
		if (SimulationCounter1 >= SIMULATION_1_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON) {
			SimulationState1 = SIMULATION_STATE_1_REST_INSIDE;
			SimulationCounter1 = 0;
			SimulationInputs[LIMIT_SWITCH_1_INDEX] = SWITCH_PRESSED;
			printf("%03u:%02u:%02u  Actuator 1   ..<<|---------\r\n",
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}
	if (SimulationState1 == SIMULATION_STATE_1_GOING_OUTSIDE) {
		SimulationCounter1++;
		if (SimulationCounter1 >= SIMULATION_1_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF) {
			SimulationState1 = SIMULATION_STATE_1_REST_OUTSIDE;
			SimulationCounter1 = 0;
			SimulationInputs[LIMIT_SWITCH_1_INDEX] = SWITCH_RELEASED;
			printf("%03u:%02u:%02u  Actuator 1   ----|>>.......\r\n", 
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}

	// 	Actuator 2 time flow simulation
	if (SimulationState2 == SIMULATION_STATE_2_GOING_INSIDE) {
		SimulationCounter2++;
		if (SimulationCounter2 >= SIMULATION_2_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON) {
			SimulationState2 = SIMULATION_STATE_2_REST_INSIDE;
			SimulationCounter2 = 0;
			SimulationInputs[LIMIT_SWITCH_2_INDEX] = SWITCH_PRESSED;
			printf("%03u:%02u:%02u  Actuator 2   ..<<|---------\r\n", 
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}
	if (SimulationState2 == SIMULATION_STATE_2_GOING_OUTSIDE) {
		SimulationCounter2++;
		if (SimulationCounter2 >= SIMULATION_2_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF) {
			SimulationState2 = SIMULATION_STATE_2_REST_OUTSIDE;
			SimulationCounter2 = 0;
			SimulationInputs[LIMIT_SWITCH_2_INDEX] = SWITCH_RELEASED;
			printf("%03u:%02u:%02u  Actuator 2   ----|>>.......\r\n", 
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}

	// 	Actuator 3 time flow simulation
	if (SimulationState3 == SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_A) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_SWITCH_B_OFF_TO_SWITCH_A_ON) {
			SimulationState3 = SIMULATION_STATE_3_REST_INSIDE;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3A_INDEX] = SWITCH_PRESSED;
			printf("%03u:%02u:%02u  Actuator 3   ..<<|-----------|----\r\n",
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}
	if (SimulationState3 == SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_B) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_B_OFF) {
			SimulationState3 = SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_A;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3B_INDEX] = SWITCH_RELEASED;
			printf("%03u:%02u:%02u  Actuator 3   ....|.........<<|----\r\n",
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}
	if (SimulationState3 == SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_B) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_SWITCH_A_OFF_TO_SWITCH_B_ON) {
			SimulationState3 = SIMULATION_STATE_3_REST_OUTSIDE;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3B_INDEX] = SWITCH_PRESSED;
			printf("%03u:%02u:%02u  Actuator 3   ----|---------|>>..\r\n", 
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}
	if (SimulationState3 == SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_A) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_INSERTED_TO_SWITCH_A_OFF) {
			SimulationState3 = SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_B;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3A_INDEX] = SWITCH_RELEASED;
			printf("%03u:%02u:%02u  Actuator 3   ----|>>.......|....\r\n", 
				SimulationClockMinutes, SimulationClockSeconds, SimulationClockMilliseconds);
		}
	}

	// Simulation clock
	SimulationClockMilliseconds += MAIN_LOOP_TICK_PERIOD_MS;
	if (SimulationClockMilliseconds >= 1000) {
		SimulationClockMilliseconds = 0;
		SimulationClockSeconds++;
		if (SimulationClockSeconds >= 60) {
			SimulationClockSeconds = 0;
			SimulationClockMinutes++;
		}
	}
}

bool simulateInput(int InputIndex){
	return SimulationInputs[InputIndex];
}

#endif // DEBUG_SIMULATION_MODE


