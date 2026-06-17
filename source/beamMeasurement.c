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

//..............................................................................
// The main routine of the project
//..............................................................................

int main() {
	// Initializations of variables, peripherals, etc.
	mainInitialization();

	printChangedRegisters("Init");

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

			printChangedRegisters("Inp. tick");

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
#if 0
		gpio_set_drive_strength(GPIO_MODBUS_ACTIVITY_LED, GPIO_DRIVE_STRENGTH_2MA);
#endif
		ModbusActiveLedTime = CurrentTime + (uint64_t)MODBUS_ACTIVITY_SHORT_TICKS;
	}
	if (ModbusActiveLedLong && !ModbusActiveLedIsOnLong) {
		ModbusActiveLedIsOnLong = true;
		gpio_put(GPIO_MODBUS_ACTIVITY_LED, true);
#if 0
		gpio_set_drive_strength(GPIO_MODBUS_ACTIVITY_LED, GPIO_DRIVE_STRENGTH_12MA);
#endif
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
	
	memset(&Inputs, 0, sizeof(Inputs));

	Inputs.installed_cups = clampInstalledCups(ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_INSTALLED_CUPS)]);
	Inputs.external_inhibition = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION2)];
	Inputs.cup_type[0] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_TYPE)];
	Inputs.cup_type[1] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_TYPE)];
	Inputs.cup_type[2] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_TYPE)];
	Inputs.time_limit_inserting_ms[0] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING1)];
	Inputs.time_limit_inserting_ms[1] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING2)];
	Inputs.time_limit_inserting_ms[2] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING3)];
	Inputs.time_limit_withdrawing_ms[0] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING1)];
	Inputs.time_limit_withdrawing_ms[1] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING2)];
	Inputs.time_limit_withdrawing_ms[2] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING3)];
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

	auxiliaryFSMsTick(&Inputs, &AuxiliaryFSMsStateData, &Outputs);

	if (Outputs.cup_error[0] != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR)]) {
		if ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_LAST_ERROR)] != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR)]) &&
			(ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR)] != 0u)) 
			{
			ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_LAST_ERROR)] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR)];
		}
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR)] = Outputs.cup_error[0];
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_ERROR_STORAGE)] |= Outputs.cup_error[0];
	}
	if (Outputs.cup_error[1] != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR)]) {
		if ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_LAST_ERROR)] != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR)]) &&
			(ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR)] != 0u)) 
			{
			ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_LAST_ERROR)] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR)];
		}
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR)] = Outputs.cup_error[1];
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_ERROR_STORAGE)] |= Outputs.cup_error[1];
	}
	if (Outputs.cup_error[2] != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR)]) {
		if ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_LAST_ERROR)] != ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR)]) &&
			(ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR)] != 0u)) 
			{
			ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_LAST_ERROR)] = ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR)];
		}
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR)] = Outputs.cup_error[2];
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_ERROR_STORAGE)] |= Outputs.cup_error[2];
	}

	if (Outputs.trigger_insert[0]) {
		printf("AuxFsmTick; actuator 1: %d->%d\r\n", ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)], Outputs.actuator_insert[0]);
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = Outputs.actuator_insert[0];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = true;
	}

	if (Outputs.trigger_insert[1]){
		printf("AuxFsmTick; actuator 2: %d->%d\r\n", ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)], Outputs.actuator_insert[1]);
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = Outputs.actuator_insert[1];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = true;
	}

	if (Outputs.trigger_insert[2]){
		printf("AuxFsmTick; actuator 3 in: %d->%d\r\n", ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)], Outputs.actuator_insert[2]);
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)] = Outputs.actuator_insert[2];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_IN)] = true;
	}
	if (Outputs.trigger_withdraw[2]){
		printf("AuxFsmTick; actuator 3 out: %d->%d\r\n", ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)], Outputs.actuator_withdraw[2]);
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)] = Outputs.actuator_withdraw[2];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_OUT)] = true;
	}
	if (Outputs.trigger_brake[2]){
		printf("AuxFsmTick; actuator 3 brake: %d->%d\r\n", ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)], Outputs.actuator_brake[2]);
		ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)] = Outputs.actuator_brake[2];
		ModbusCoilTrigger[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL_BRAKE)] = true;
	}
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

