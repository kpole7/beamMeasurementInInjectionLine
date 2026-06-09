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
#define SIMULATION_1_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON		(600ul / MAIN_LOOP_TICK_PERIOD_MS)  // 600 ms
#define SIMULATION_1_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF		(200ul / MAIN_LOOP_TICK_PERIOD_MS)  // 200 ms
#define SIMULATION_2_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON		(600ul / MAIN_LOOP_TICK_PERIOD_MS)  // 600 ms
#define SIMULATION_2_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF		(200ul / MAIN_LOOP_TICK_PERIOD_MS)  // 200 ms

#define SIMULATION_3_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_B_OFF		(100ul / MAIN_LOOP_TICK_PERIOD_MS)  // 100 ms
#define SIMULATION_3_PROPAGATION_FROM_SWITCH_B_OFF_TO_SWITCH_A_ON	(3000ul / MAIN_LOOP_TICK_PERIOD_MS) // 3 s
#define SIMULATION_3_PROPAGATION_FROM_INSERTED_TO_SWITCH_A_OFF		(200ul / MAIN_LOOP_TICK_PERIOD_MS)  // 200 ms
#define SIMULATION_3_PROPAGATION_FROM_SWITCH_A_OFF_TO_SWITCH_B_ON	(4000ul / MAIN_LOOP_TICK_PERIOD_MS) // 4 s

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

#define SWITCH_PRESSED true
#define SWITCH_RELEASED false

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

uint16_t TimeStampMinutes;
uint16_t TimeStampSeconds;
uint16_t TimeStampMilliseconds;
static char TimeStampString[16];

#if DEBUG_SIMULATION_MODE

// This table is used in the debug simulation mode to simulate the state of logic inputs.
// Indexes are defined in logicInputs.h
// SimulationInputs[EXTERNAL_INHIBITION_INDEX] is intentionally not used
bool SimulationInputs[5] = { SWITCH_RELEASED, SWITCH_RELEASED, SWITCH_RELEASED, SWITCH_PRESSED, false };

uint16_t SimulationState1;
uint16_t SimulationState2;
uint16_t SimulationState3;

uint16_t SimulationCounter1;
uint16_t SimulationCounter2;
uint16_t SimulationCounter3;

#endif // DEBUG_SIMULATION_MODE


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

void initializeTimeStamp(void){
	TimeStampMinutes = 0;
	TimeStampSeconds = 0;
	TimeStampMilliseconds = 0;
}

void updateTimeStamp( uint16_t MillisecondsToAdd ) {
	TimeStampMilliseconds += MillisecondsToAdd;
	if (TimeStampMilliseconds >= 1000) {
		TimeStampMilliseconds = 0;
		TimeStampSeconds++;
		if (TimeStampSeconds >= 60) {
			TimeStampSeconds = 0;
			TimeStampMinutes++;
			if (TimeStampMinutes >= 60*12) { // after 12 hours, reset the time stamp to avoid overflow
				TimeStampMinutes = 0;
			}
		}
	}
}

char *getTimeStampString(void){
	snprintf(TimeStampString, sizeof(TimeStampString), "%03u:%02u:%03u", TimeStampMinutes, TimeStampSeconds, TimeStampMilliseconds);
	return TimeStampString;
}

char *getTimeStampStringWithoutUpdate(void){
	return TimeStampString;
}

void printChangedRegisters( const char *ContextComment ) {
	static uint16_t OldModbusHoldingRegisters[MODBUS_HOLDING_REGISTERS_NUMBER];
	static bool OldModbusCoils[MODBUS_COILS_NUMBER];
	bool AnyChange = false;
	int16_t Counter = 3;

	for (int J = 0; J < MODBUS_COILS_NUMBER; J++) {
		if (ModbusCoils[J] != OldModbusCoils[J]) {
			if (!AnyChange) {
				printf("%s %11s  ", getTimeStampString(), ContextComment);
			}
			AnyChange = true;
			printf("%02d: %u->%u  ", J+MODBUS_COILS_ADDRESS, OldModbusCoils[J], ModbusCoils[J]);
			OldModbusCoils[J] = ModbusCoils[J];
			Counter++;
		}
	}
	if (AnyChange && (Counter % 2 == 0)) {
		Counter++;
		printf("          ");
	}
	for (int J = 0; J < MODBUS_HOLDING_REGISTERS_NUMBER; J++) {
		if (ModbusHoldingRegisters[J] != OldModbusHoldingRegisters[J]) {
			if (!AnyChange) {
				printf("%s %11s  ", getTimeStampString(), ContextComment);
			}
			AnyChange = true;
			printf("%04d: %04X -> %04X  ", J+MODBUS_HOLDING_REGISTERS_ADDRESS, OldModbusHoldingRegisters[J], ModbusHoldingRegisters[J]);
			OldModbusHoldingRegisters[J] = ModbusHoldingRegisters[J];
			Counter+=2;
			if (Counter >= 16) {
				printf("\r\n    ");
				Counter = 0;
			}
		}
	}
	if (AnyChange) {
		printf("\r\n");
	}
}

#if DEBUG_SIMULATION_MODE

void simulationMainLoopTick(void){
	(void)getTimeStampString(); // Update the time stamp string for printouts.

	// 	Actuator 1 event simulation
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)]){
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)]) {
			if (SimulationState1 != SIMULATION_STATE_1_REST_OUTSIDE) {
				printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState1);
			}
			else{
				SimulationState1 = SIMULATION_STATE_1_GOING_INSIDE;
				SimulationCounter1 = 0;
				printf("%s  Sim  Actuator 1   ....|.........<<\r\n", getTimeStampStringWithoutUpdate());
			}
		}
		else {
			if (SimulationState1 != SIMULATION_STATE_1_REST_INSIDE) {
				printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState1);
			}
			else{
				SimulationState1 = SIMULATION_STATE_1_GOING_OUTSIDE;
				SimulationCounter1 = 0;
				printf("%s  Sim  Actuator 1   >>..|.........\r\n", getTimeStampStringWithoutUpdate());
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = false;
	}

	// 	Actuator 2 event simulation
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)]){
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)]) {
			if (SimulationState2 != SIMULATION_STATE_2_REST_OUTSIDE) {
				printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState2);
			}
			else{
				SimulationState2 = SIMULATION_STATE_2_GOING_INSIDE;
				SimulationCounter2 = 0;
				printf("%s  Sim  Actuator 2   ....|.........<<\r\n", getTimeStampStringWithoutUpdate());
			}
		}
		else {
			if (SimulationState2 != SIMULATION_STATE_2_REST_INSIDE) {
				printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState2);
			}
			else{
				SimulationState2 = SIMULATION_STATE_2_GOING_OUTSIDE;
				SimulationCounter2 = 0;
				printf("%s  Sim  Actuator 2   >>..|.........\r\n", getTimeStampStringWithoutUpdate());
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = false;
	}

	// 	Actuator 3 event simulation: insert
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)]){
		// Request to insert or to stop inserting the cup 3
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)]) {
			printf("%s  Sim  ERROR in line %u, conflict: Actuator3ControlOut is set\r\n", getTimeStampStringWithoutUpdate(), __LINE__);
		}
		else{
			if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)]) {
				// Request to insert
				if (SimulationState3 != SIMULATION_STATE_3_REST_OUTSIDE) {
					printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState3);
				}
				else{
					SimulationState3 = SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_B;
					SimulationCounter3 = 0;
					printf("%s  Sim  Actuator 3   ....|...........|..<< new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
				}
			}
			else {
				// Request to stop inserting
				if (SimulationState3 != SIMULATION_STATE_3_REST_INSIDE) {
					printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState3);
				}
				else{
					printf("%s  Sim  Actuator 3   []..|...........|.... new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
				}
			}
		}
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)] = false;
	}

	// 	Actuator 3 event simulation: extract
	if (ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)]){
		// Request to extract or to stop extracting the cup 3
		if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)]) {
			printf("%s  Sim  ERROR in line %u, conflict: Actuator3ControlIn is set\r\n", getTimeStampStringWithoutUpdate(), __LINE__);
		}
		else{
			if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)]) {
				// Request to extract
				if (SimulationState3 != SIMULATION_STATE_3_REST_INSIDE) {
					printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState3);
				}
				else{
					SimulationState3 = SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_A;
					SimulationCounter3 = 0;
					printf("%s  Sim  Actuator 3   >>..|...........|.... new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
				}
			}
			else {
				// Request to stop extracting
				if (SimulationState3 != SIMULATION_STATE_3_REST_OUTSIDE) {
					printf("%s  Sim  Warning, Line %u, unexpected state %u\r\n", getTimeStampStringWithoutUpdate(), __LINE__, SimulationState3);
				}
				else{
					printf("%s  Sim  Actuator 3   ....|...........|..[] new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
				}
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
			printf("%s  Sim  Actuator 1   ..<<|---------\r\n", getTimeStampStringWithoutUpdate());
		}
	}
	if (SimulationState1 == SIMULATION_STATE_1_GOING_OUTSIDE) {
		SimulationCounter1++;
		if (SimulationCounter1 >= SIMULATION_1_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF) {
			SimulationState1 = SIMULATION_STATE_1_REST_OUTSIDE;
			SimulationCounter1 = 0;
			SimulationInputs[LIMIT_SWITCH_1_INDEX] = SWITCH_RELEASED;
			printf("%s  Sim  Actuator 1   ----|>>.......\r\n", getTimeStampStringWithoutUpdate());
		}
	}

	// 	Actuator 2 time flow simulation
	if (SimulationState2 == SIMULATION_STATE_2_GOING_INSIDE) {
		SimulationCounter2++;
		if (SimulationCounter2 >= SIMULATION_2_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_ON) {
			SimulationState2 = SIMULATION_STATE_2_REST_INSIDE;
			SimulationCounter2 = 0;
			SimulationInputs[LIMIT_SWITCH_2_INDEX] = SWITCH_PRESSED;
			printf("%s  Sim  Actuator 2   ..<<|---------\r\n", getTimeStampStringWithoutUpdate());
		}
	}
	if (SimulationState2 == SIMULATION_STATE_2_GOING_OUTSIDE) {
		SimulationCounter2++;
		if (SimulationCounter2 >= SIMULATION_2_PROPAGATION_FROM_INSERTED_TO_SWITCH_OFF) {
			SimulationState2 = SIMULATION_STATE_2_REST_OUTSIDE;
			SimulationCounter2 = 0;
			SimulationInputs[LIMIT_SWITCH_2_INDEX] = SWITCH_RELEASED;
			printf("%s  Sim  Actuator 2   ----|>>.......\r\n", getTimeStampStringWithoutUpdate());
		}
	}

	// 	Actuator 3 time flow simulation
	if (SimulationState3 == SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_A) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_SWITCH_B_OFF_TO_SWITCH_A_ON) {
			SimulationState3 = SIMULATION_STATE_3_REST_INSIDE;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3A_INDEX] = SWITCH_PRESSED;
			printf("%s  Sim  Actuator 3   ..<<|-----------|---- new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
		}
	}
	if (SimulationState3 == SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_B) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_EXTRACTED_TO_SWITCH_B_OFF) {
			SimulationState3 = SIMULATION_STATE_3_GOING_INSIDE_TO_SWITCH_A;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3B_INDEX] = SWITCH_RELEASED;
			printf("%s  Sim  Actuator 3   ....|.........<<|---- new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
		}
	}
	if (SimulationState3 == SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_B) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_SWITCH_A_OFF_TO_SWITCH_B_ON) {
			SimulationState3 = SIMULATION_STATE_3_REST_OUTSIDE;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3B_INDEX] = SWITCH_PRESSED;
			printf("%s  Sim  Actuator 3   ----|-----------|>>.. new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
		}
	}
	if (SimulationState3 == SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_A) {
		SimulationCounter3++;
		if (SimulationCounter3 >= SIMULATION_3_PROPAGATION_FROM_INSERTED_TO_SWITCH_A_OFF) {
			SimulationState3 = SIMULATION_STATE_3_GOING_OUTSIDE_TO_SWITCH_B;
			SimulationCounter3 = 0;
			SimulationInputs[LIMIT_SWITCH_3A_INDEX] = SWITCH_RELEASED;
			printf("%s  Sim  Actuator 3   ----|>>.........|.... new state=%u\r\n", getTimeStampStringWithoutUpdate(), SimulationState3);
		}
	}
	// SimulationInputs[EXTERNAL_INHIBITION_INDEX] is intentionally not used
}

bool simulateInput(int InputIndex){
	return SimulationInputs[InputIndex];
}

#endif // DEBUG_SIMULATION_MODE


