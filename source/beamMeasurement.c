/// @file beamMeasurement.c
/// This source code file was written by K.O. (2025 - 2026)
///
///
/// Abbreviations:
/// PSU = power supply unit
/// FSM = finite state machine
/// ISR = interrupt service routine

#include "analogInputs.h"
#include "logicInputs.h"
#include "auxiliaryFSMs.h"
#include "compilationTime.h"
#include "debuggingTools.h"
#include "highLevelCtrl.h"
#include "mainTimer.h"
#include "masterConfig.h"
#include "mb.h"
#include "modbusConfig.h"
#include "sharedData.h"
#include "actuatorCtrl.h"
#include "pico/stdlib.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/// This directive tells that the LED on pico PCB is connected to GPIO25 port
#define GPIO_FOR_PICO_ON_BOARD_LED 25

#define FAST_PERIPHERALS_TICK_PERIOD_MS 2u

//..............................................................................
// Local variables concerning checking jumper states and LED light time
//..............................................................................

static bool ModbusActiveLedIsOnShort, ModbusActiveLedIsOnLong;

//..............................................................................
// Variables for debugging
//..............................................................................

#if MODBUS_DEBUG_MODE
bool IsChangeModbusWrite;
static bool IsJumperJP1;
static bool OldIsJumperJP1;
#endif

//..............................................................................
// Prototypes of functions
//..............................................................................

/// This function initializes and turns on the LED on pico board.
static void turnOnLedOnBoard(void);

/// This function initializes the operation of the LED that indicates Modbus transmission.
static void initModbusActivityLed(void);

/// This function supports the LED that indicates Modbus transmission (provides
/// the correct timing and strength of illumination).
static void modbusActivityLedService(void);

static void printRegisters(void);

static void mainInitialization(void);

static void highLevelCtrlService(void);

static void auxiliaryFSMsService(void);

static uint16_t clampInstalledCups(uint16_t value);

static uint16_t clampActiveCup(uint16_t value);

static HighLevelCtrlState HighLevelState;
static bool IsHighLevelStateInitialized;

static AuxiliaryFSMsState AuxiliaryFSMsStateData;
static bool IsAuxiliaryFSMsStateInitialized;

//..............................................................................
// The main routine of the project
//..............................................................................

int main() {
	// Initializations of variables, peripherals, etc.
	mainInitialization();

	//...............
	// The main loop
	//...............

	while (1) {
		if (atomic_load_explicit(&SlowProcessesTimeTick1, memory_order_acquire)) {
			atomic_store_explicit(&SlowProcessesTimeTick1, false, memory_order_release);

			getVoltageSamples();
		}

		if (atomic_load_explicit(&TwoMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&TwoMillisecondsTimeTick, false, memory_order_release);

			logicInputsTick();
			modbusActivityLedService();
#if 0
			highLevelCtrlService();
#endif
			auxiliaryFSMsService();

#if DEBUG_SIMULATION_MODE == 0
			actuatorCtrlTick();
#else
			simulationMainLoopTick();
#endif

	updateTimeStamp(FAST_PERIPHERALS_TICK_PERIOD_MS);

#if MODBUS_DEBUG_MODE
			// Reading the states of jumpers.
			IsJumperJP1 = !readInputPortJP1(); // false;	// Modbus state machine debugging
#endif
		}

		printChangedRegisters("Main loop");

#if MODBUS_DEBUG_MODE
		// Auxiliary printouts for debugging purpose
		if (IsJumperJP1 && !OldIsJumperJP1) {
			logPrintAll(0);
		}
		OldIsJumperJP1 = IsJumperJP1;
#endif

		// Modbus communication service
		if (!atomic_load_explicit(&ModbusAssertionFailed, memory_order_acquire)) {

			eMBPoll();
		}
		else {
			// Stopping the modbus state machine instead of executing the function 'assert'
			// that was in the original freemodbus source code.
			// That enables failover and debugger actions.
			if (irq_is_enabled(MODBUS_UART_IRQ)) {
				irq_set_enabled(MODBUS_UART_IRQ, false);
			}
		}

		printChangedRegisters("Modbus");

	} // The main loop
}

//..............................................................................
// Definitions of functions
//..............................................................................

/// This function initializes and turns on the LED on pico board.
static void turnOnLedOnBoard(void) {
	gpio_init(GPIO_FOR_PICO_ON_BOARD_LED);
	gpio_set_dir(GPIO_FOR_PICO_ON_BOARD_LED, GPIO_OUT);
	gpio_put(GPIO_FOR_PICO_ON_BOARD_LED, true);
}

/// This function initializes the operation of the LED that indicates Modbus transmission.
static void initModbusActivityLed(void) {
	gpio_init(GPIO_MODBUS_ACTIVITY_LED);
	gpio_set_dir(GPIO_MODBUS_ACTIVITY_LED, GPIO_OUT);
	gpio_put(GPIO_MODBUS_ACTIVITY_LED, false);

	ModbusActiveLedIsOnShort = false;
	ModbusActiveLedIsOnLong = false;
	atomic_store_explicit(&ModbusActiveLedShort, false, memory_order_release);
	ModbusActiveLedLong = false;
}

/// This function supports the LED that indicates Modbus transmission (provides
/// the correct timing and strength of illumination).
static void modbusActivityLedService(void) {
	static uint64_t ModbusActiveLedTime;
	static uint64_t CurrentTime;
	CurrentTime = time_us_64();
	if (atomic_load_explicit(&ModbusActiveLedShort, memory_order_acquire) && !ModbusActiveLedIsOnShort) {
		ModbusActiveLedIsOnShort = true;
		gpio_put(GPIO_MODBUS_ACTIVITY_LED, true);
		gpio_set_drive_strength(GPIO_MODBUS_ACTIVITY_LED, GPIO_DRIVE_STRENGTH_2MA);
		ModbusActiveLedTime = CurrentTime + (uint64_t)MODBUS_ACTIVITY_SHORT_TICKS;
	}
	if (ModbusActiveLedLong && !ModbusActiveLedIsOnLong) {
		ModbusActiveLedIsOnLong = true;
		gpio_put(GPIO_MODBUS_ACTIVITY_LED, true);
		gpio_set_drive_strength(GPIO_MODBUS_ACTIVITY_LED, GPIO_DRIVE_STRENGTH_12MA);
		ModbusActiveLedTime = CurrentTime + (uint64_t)MODBUS_ACTIVITY_LONG_TICKS;
	}
	if (CurrentTime > ModbusActiveLedTime) {
		gpio_put(GPIO_MODBUS_ACTIVITY_LED, false);
		ModbusActiveLedIsOnShort = false;
		ModbusActiveLedIsOnLong = false;
		atomic_store_explicit(&ModbusActiveLedShort, false, memory_order_release);
		ModbusActiveLedLong = false;
	}
}

static void printRegisters(void){
	for (int J = 0; J < MODBUS_INPUT_REGISTERS_NUMBER; J++) {
		printf("  %04X", ModbusInputRegisters[J]);
		if ((J % 5) == 4) {
			printf("\r\n");
		}
	}
	for (int J = 0; J < MODBUS_COILS_NUMBER; J++) {
		printf(" %c", ModbusCoils[J] ? '1' : '0');
		if ((J % 3) == 2) {
			printf(" |");
		}
	}
	printf("\r\n");
}

static void mainInitialization(void){
	stdio_init_all();
	atomic_store_explicit(&ModbusAssertionFailed, false, memory_order_release);
	turnOnLedOnBoard();
	initModbusActivityLed();
	initializeModbusRegisters();

	memset(&HighLevelState, 0, sizeof(HighLevelState));
	HighLevelState.retained_active_cup = 1u;
	IsHighLevelStateInitialized = false;

	memset(&AuxiliaryFSMsStateData, 0, sizeof(AuxiliaryFSMsStateData));
	IsAuxiliaryFSMsStateInitialized = false;

#if MODBUS_DEBUG_MODE
	initInputPortJP1();
	initAuxiliaryPrintouts();
#endif

	sleep_ms(100);
	printf("\r\nHello!\r\nCompilation time is %s\r\n", CompilationTime);
#if DEBUG_SIMULATION_MODE
	printf("Simulation mode is ON\r\n");
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] = 4u; // Logic Inputs printouts enabled
#else
	printf("Simulation mode is OFF\r\n");
#endif

	initializeLogicInputs();
	initializeAdcMeasurements();
	auxiliaryOutputsInitialize();
	initializeActuatorControl();
	initializeTimeStamp();

	eMBInit(MB_RTU, MODBUS_SLAVE_ID, 0, MODBUS_BAUD_RATE, MODBUS_PARITY); // The parameter ucPort is a dummy and will be ignored
	eMBEnable();

	startPeriodicInterrupt();

	printChangedRegisters("Init");
}

static void highLevelCtrlService(void) {
	HighLevelCtrlInputs Inputs;
	HighLevelCtrlOutputs Outputs;
	uint16_t ErrorStorageIndex = holdingIndexFromAddress(MODBUS_ADDR_ERROR_STORAGE);
	uint16_t ActiveCupIndex = holdingIndexFromAddress(MODBUS_ADDR_ACTIVE_CUP);
	uint16_t ErrorCodeIndex = holdingIndexFromAddress(MODBUS_ADDR_ERROR_CODE);
	uint16_t LastErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_LAST_ERROR);

	memset(&Inputs, 0, sizeof(Inputs));

	Inputs.installed_cups = clampInstalledCups(ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_INSTALLED_CUPS)]);
	Inputs.external_inhibition = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION2)];
	Inputs.cup_control[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_CONTROL)];
	Inputs.cup_control[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_CONTROL)];
	Inputs.cup_control[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_CONTROL)];
	Inputs.cup_error[0] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR)];
	Inputs.cup_error[1] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR)];
	Inputs.cup_error[2] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR)];
	Inputs.cup_steady[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_STEADY)];
	Inputs.cup_steady[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_STEADY)];
	Inputs.cup_steady[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_STEADY)];
	Inputs.cup_inserted[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_INSERTED)];
	Inputs.cup_inserted[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_INSERTED)];
	Inputs.cup_inserted[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_INSERTED)];

	if (!IsHighLevelStateInitialized) {
		HighLevelState.prev_error_code = ModbusHoldingRegisters[ErrorCodeIndex];
		HighLevelState.retained_active_cup = clampActiveCup(ModbusHoldingRegisters[ActiveCupIndex]);
		HighLevelState.error_storage = ModbusHoldingRegisters[ErrorStorageIndex];
		IsHighLevelStateInitialized = true;
	}

	// Allow reset from Modbus write in normal/debug mode.
	HighLevelState.error_storage = ModbusHoldingRegisters[ErrorStorageIndex];

	highLevelCtrlTick(&Inputs, &HighLevelState, &Outputs);

	ModbusHoldingRegisters[ErrorCodeIndex] = Outputs.error_code;
	if (Outputs.last_error != 0u) {
		ModbusHoldingRegisters[LastErrorIndex] = Outputs.last_error;
	}
	ModbusHoldingRegisters[ErrorStorageIndex] = Outputs.error_storage;
	ModbusHoldingRegisters[ActiveCupIndex] = clampActiveCup(Outputs.active_cup);

	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_REQUESTED_STATE)] = Outputs.cup_requested_state[0];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_REQUESTED_STATE)] = Outputs.cup_requested_state[1];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_REQUESTED_STATE)] = Outputs.cup_requested_state[2];
}

static void auxiliaryFSMsService(void) {
	AuxiliaryFSMsInputs Inputs;
	AuxiliaryFSMsOutputs Outputs;
	uint16_t InstalledCupsIndex = holdingIndexFromAddress(MODBUS_ADDR_INSTALLED_CUPS);
	uint16_t Cup1TypeIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP1_TYPE);
	uint16_t Cup2TypeIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP2_TYPE);
	uint16_t Cup3TypeIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP3_TYPE);
	uint16_t TimeLimitInserting1Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING1);
	uint16_t TimeLimitInserting2Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING2);
	uint16_t TimeLimitInserting3Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING3);
	uint16_t TimeLimitWithdrawing1Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING1);
	uint16_t TimeLimitWithdrawing2Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING2);
	uint16_t TimeLimitWithdrawing3Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING3);
	uint16_t Cup1ErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR);
	uint16_t Cup2ErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR);
	uint16_t Cup3ErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR);
	uint16_t Cup1LastErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP1_LAST_ERROR);
	uint16_t Cup2LastErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP2_LAST_ERROR);
	uint16_t Cup3LastErrorIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP3_LAST_ERROR);
	uint16_t Cup1ErrorStorageIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR_STORAGE);
	uint16_t Cup2ErrorStorageIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR_STORAGE);
	uint16_t Cup3ErrorStorageIndex = holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR_STORAGE);

	memset(&Inputs, 0, sizeof(Inputs));

	Inputs.installed_cups = clampInstalledCups(ModbusHoldingRegisters[InstalledCupsIndex]);
	Inputs.external_inhibition = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION2)];
	Inputs.cup_type[0] = ModbusHoldingRegisters[Cup1TypeIndex];
	Inputs.cup_type[1] = ModbusHoldingRegisters[Cup2TypeIndex];
	Inputs.cup_type[2] = ModbusHoldingRegisters[Cup3TypeIndex];
	Inputs.time_limit_inserting_ms[0] = ModbusHoldingRegisters[TimeLimitInserting1Index];
	Inputs.time_limit_inserting_ms[1] = ModbusHoldingRegisters[TimeLimitInserting2Index];
	Inputs.time_limit_inserting_ms[2] = ModbusHoldingRegisters[TimeLimitInserting3Index];
	Inputs.time_limit_withdrawing_ms[0] = ModbusHoldingRegisters[TimeLimitWithdrawing1Index];
	Inputs.time_limit_withdrawing_ms[1] = ModbusHoldingRegisters[TimeLimitWithdrawing2Index];
	Inputs.time_limit_withdrawing_ms[2] = ModbusHoldingRegisters[TimeLimitWithdrawing3Index];
	Inputs.cup_requested_state[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_REQUESTED_STATE)];
	Inputs.cup_requested_state[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_REQUESTED_STATE)];
	Inputs.cup_requested_state[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_REQUESTED_STATE)];
	Inputs.cup_switch[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)];
	Inputs.cup_switch[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)];
	Inputs.cup_switch[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH_A)];
	Inputs.cup_switch_a[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)];
	Inputs.cup_switch_a[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)];
	Inputs.cup_switch_a[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH_A)];
	Inputs.cup_switch_b[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)];
	Inputs.cup_switch_b[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)];
	Inputs.cup_switch_b[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH_B)];

	if (!IsAuxiliaryFSMsStateInitialized) {
		AuxiliaryFSMsStateData.prev_error[0] = ModbusHoldingRegisters[Cup1ErrorIndex];
		AuxiliaryFSMsStateData.prev_error[1] = ModbusHoldingRegisters[Cup2ErrorIndex];
		AuxiliaryFSMsStateData.prev_error[2] = ModbusHoldingRegisters[Cup3ErrorIndex];
		AuxiliaryFSMsStateData.error_storage[0] = ModbusHoldingRegisters[Cup1ErrorStorageIndex];
		AuxiliaryFSMsStateData.error_storage[1] = ModbusHoldingRegisters[Cup2ErrorStorageIndex];
		AuxiliaryFSMsStateData.error_storage[2] = ModbusHoldingRegisters[Cup3ErrorStorageIndex];
		IsAuxiliaryFSMsStateInitialized = true;
	}

	AuxiliaryFSMsStateData.error_storage[0] = ModbusHoldingRegisters[Cup1ErrorStorageIndex];
	AuxiliaryFSMsStateData.error_storage[1] = ModbusHoldingRegisters[Cup2ErrorStorageIndex];
	AuxiliaryFSMsStateData.error_storage[2] = ModbusHoldingRegisters[Cup3ErrorStorageIndex];

	auxiliaryFSMsTick(&Inputs, &AuxiliaryFSMsStateData, &Outputs, FAST_PERIPHERALS_TICK_PERIOD_MS);

	ModbusHoldingRegisters[Cup1ErrorIndex] = Outputs.cup_error[0];
	ModbusHoldingRegisters[Cup2ErrorIndex] = Outputs.cup_error[1];
	ModbusHoldingRegisters[Cup3ErrorIndex] = Outputs.cup_error[2];
	if (Outputs.cup_last_error[0] != 0u) {
		ModbusHoldingRegisters[Cup1LastErrorIndex] = Outputs.cup_last_error[0];
	}
	if (Outputs.cup_last_error[1] != 0u) {
		ModbusHoldingRegisters[Cup2LastErrorIndex] = Outputs.cup_last_error[1];
	}
	if (Outputs.cup_last_error[2] != 0u) {
		ModbusHoldingRegisters[Cup3LastErrorIndex] = Outputs.cup_last_error[2];
	}
	ModbusHoldingRegisters[Cup1ErrorStorageIndex] = Outputs.cup_error_storage[0];
	ModbusHoldingRegisters[Cup2ErrorStorageIndex] = Outputs.cup_error_storage[1];
	ModbusHoldingRegisters[Cup3ErrorStorageIndex] = Outputs.cup_error_storage[2];

	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_STEADY)] != Outputs.cup_steady[0]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_STEADY)] = Outputs.cup_steady[0];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_CUP1_STEADY)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_STEADY)] != Outputs.cup_steady[1]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_STEADY)] = Outputs.cup_steady[1];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_CUP2_STEADY)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_STEADY)] != Outputs.cup_steady[2]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_STEADY)] = Outputs.cup_steady[2];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_CUP3_STEADY)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_INSERTED)] != Outputs.cup_inserted[0]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_INSERTED)] = Outputs.cup_inserted[0];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_CUP1_INSERTED)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_INSERTED)] != Outputs.cup_inserted[1]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_INSERTED)] = Outputs.cup_inserted[1];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_CUP2_INSERTED)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_INSERTED)] != Outputs.cup_inserted[2]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_INSERTED)] = Outputs.cup_inserted[2];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_CUP3_INSERTED)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] != Outputs.actuator_control[0]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = Outputs.actuator_control[0];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = true;
	}
	if (ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] != Outputs.actuator_control[1]){
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = Outputs.actuator_control[1];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = true;
	}

//	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL)] = Outputs.actuator_control[2];




}

static uint16_t clampInstalledCups(uint16_t value) {
	if (value < 1u) {
		return 1u;
	}
	if (value > 3u) {
		return 3u;
	}
	return value;
}

static uint16_t clampActiveCup(uint16_t value) {
	if (value < 1u) {
		return 1u;
	}
	if (value > 3u) {
		return 3u;
	}
	return value;
}

