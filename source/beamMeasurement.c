/// @file beamMeasurement.c
/// This source code file was written by K.O. (2025 - 2026)
///
///
/// Abbreviations:
/// PSU = power supply unit
/// FSM = finite state machine
/// ISR = interrupt service routine

#include "adcInputs.h"
#include "auxiliaryFSMs.h"
#include "compilationTime.h"
#include "debuggingTools.h"
#include "highLevelCtrl.h"
#include "mainTimer.h"
#include "masterConfig.h"
#include "mb.h"
#include "modbusConfig.h"
#include "sharedData.h"
#include "pico/stdlib.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/// This directive tells that the LED on pico PCB is connected to GPIO25 port
#define PICO_ON_BOARD_LED_PIN 25

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

static void slowProcessesService(void);

static void highLevelCtrlService(void);

static void auxiliaryFSMsService(void);

static uint16_t holdingIndexFromAddress(uint16_t address);

static uint16_t inputIndexFromAddress(uint16_t address);

static uint16_t coilIndexFromAddress(uint16_t address);

static uint16_t clampInstalledCups(uint16_t value);

static uint16_t clampActiveCup(uint16_t value);

static HighLevelCtrlState HighLevelState;
static bool IsHighLevelStateInitialized;

static AuxiliaryFSMsState AuxiliaryFSMsStateData;
static bool IsAuxiliaryFSMsStateInitialized;

#define AUXILIARY_FSM_TICK_PERIOD_MS 2u

//..............................................................................
// The main routine of the project
//..............................................................................

int main() {
	// Initializations of variables, peripherals, etc.
	mainInitialization();

#if 0  // Auxiliary printouts for debugging purpose
	sleep_ms(1000);
	printf("Hello!\r\n");
#endif

	//...............
	// The main loop
	//...............

	while (1) {

		if (atomic_load_explicit(&SixtyFourMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&SixtyFourMillisecondsTimeTick, false, memory_order_release);

			slowProcessesService();
		}

		if (atomic_load_explicit(&TwoMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&TwoMillisecondsTimeTick, false, memory_order_release);

			modbusActivityLedService();
			highLevelCtrlService();
			auxiliaryFSMsService();

#if MODBUS_DEBUG_MODE
			// Reading the states of jumpers.
			IsJumperJP1 = !readInputPortJP1(); // false;	// Modbus state machine debugging
#endif
		}

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
			if (irq_is_enabled(MODBUS_UART0_IRQ)) {
				irq_set_enabled(MODBUS_UART0_IRQ, false);
			}
		}
	} // The main loop
}

//..............................................................................
// Definitions of functions
//..............................................................................

/// This function initializes and turns on the LED on pico board.
static void turnOnLedOnBoard(void) {
	gpio_init(PICO_ON_BOARD_LED_PIN);
	gpio_set_dir(PICO_ON_BOARD_LED_PIN, GPIO_OUT);
	gpio_put(PICO_ON_BOARD_LED_PIN, true);
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
	uint16_t InstalledCupsAddress = holdingIndexFromAddress(MODBUS_ADDR_INSTALLED_CUPS);
	uint16_t ElectrodesInsideCup1Address = holdingIndexFromAddress(MODBUS_ADDR_ELECTRODES_CUP1);
	uint16_t ElectrodesInsideCup2Address = holdingIndexFromAddress(MODBUS_ADDR_ELECTRODES_CUP2);
	uint16_t ElectrodesInsideCup3Address = holdingIndexFromAddress(MODBUS_ADDR_ELECTRODES_CUP3);
	uint16_t Cup1TypeAddress = holdingIndexFromAddress(MODBUS_ADDR_CUP1_TYPE);
	uint16_t Cup2TypeAddress = holdingIndexFromAddress(MODBUS_ADDR_CUP2_TYPE);
	uint16_t Cup3TypeAddress = holdingIndexFromAddress(MODBUS_ADDR_CUP3_TYPE);
	uint16_t ActiveCupAddress = holdingIndexFromAddress(MODBUS_ADDR_ACTIVE_CUP);

	atomic_store_explicit(&ModbusAssertionFailed, false, memory_order_release);

	stdio_init_all();
	turnOnLedOnBoard();
	initModbusActivityLed();
	initializeAdcMeasurements();

	for (int J = 0; J < MODBUS_HOLDING_REGISTERS_NUMBER; J++) {
		ModbusHoldingRegisters[J] = 0;
	}
	for (int J = 0; J < MODBUS_COILS_NUMBER; J++) {
		ModbusCoils[J] = false;
		CoilsChanged[J] = false;
	}

	// Defaults from ModbusRegisters.csv
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_CONTROL)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_CONTROL)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_CONTROL)] = true;
	ModbusHoldingRegisters[InstalledCupsAddress] = 3u;
	ModbusHoldingRegisters[ElectrodesInsideCup1Address] = 4u;
	ModbusHoldingRegisters[ElectrodesInsideCup2Address] = 4u;
	ModbusHoldingRegisters[ElectrodesInsideCup3Address] = 4u;
	ModbusHoldingRegisters[Cup1TypeAddress] = 0u;
	ModbusHoldingRegisters[Cup2TypeAddress] = 0u;
	ModbusHoldingRegisters[Cup3TypeAddress] = 1u;
	ModbusHoldingRegisters[ActiveCupAddress] = 1u;

	memset(&HighLevelState, 0, sizeof(HighLevelState));
	HighLevelState.retained_active_cup = 1u;
	IsHighLevelStateInitialized = false;

	memset(&AuxiliaryFSMsStateData, 0, sizeof(AuxiliaryFSMsStateData));
	IsAuxiliaryFSMsStateInitialized = false;

#if MODBUS_DEBUG_MODE
	initInputPortJP1();
	initAuxiliaryPrintouts();
#endif
	initInputPortJP2();
	printf("\r\nHello!\r\nCompilation time is %s\r\n", CompilationTime);

	auxiliaryOutputsInitialize();

	eMBInit(MB_RTU, MODBUS_SLAVE_ID, 0, MODBUS_BAUD_RATE, MODBUS_PARITY); // The parameter ucPort is a dummy and will be ignored
	eMBEnable();

	startPeriodicInterrupt();
}

/// This function is used for debugging purposes
static void slowProcessesService(void){
	debugTerminalCommandInterpreter(&ModbusHoldingRegisters[0], MODBUS_HOLDING_REGISTERS_NUMBER, 'a');
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
	Inputs.external_inhibition = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION)];
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
	uint16_t TimeLimitInserting1Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING_1);
	uint16_t TimeLimitInserting2Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING_2);
	uint16_t TimeLimitInserting3Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING_3);
	uint16_t TimeLimitWithdrawing1Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING_1);
	uint16_t TimeLimitWithdrawing2Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING_2);
	uint16_t TimeLimitWithdrawing3Index = holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING_3);
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
	Inputs.external_inhibition = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION)];
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
	Inputs.cup_switch[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH2)];
	Inputs.cup_switch1[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)];
	Inputs.cup_switch1[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)];
	Inputs.cup_switch1[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH1)];
	Inputs.cup_switch2[0] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)];
	Inputs.cup_switch2[1] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)];
	Inputs.cup_switch2[2] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH2)];

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

	auxiliaryFSMsTick(&Inputs, &AuxiliaryFSMsStateData, &Outputs, AUXILIARY_FSM_TICK_PERIOD_MS);

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

	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_STEADY)] = Outputs.cup_steady[0];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_STEADY)] = Outputs.cup_steady[1];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_STEADY)] = Outputs.cup_steady[2];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_INSERTED)] = Outputs.cup_inserted[0];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_INSERTED)] = Outputs.cup_inserted[1];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_INSERTED)] = Outputs.cup_inserted[2];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = Outputs.actuator_control[0];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = Outputs.actuator_control[1];
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR3_CONTROL)] = Outputs.actuator_control[2];
}

static uint16_t holdingIndexFromAddress(uint16_t address) {
	return (uint16_t)(address - MODBUS_HOLDING_REGISTERS_ADDRESS);
}

static uint16_t inputIndexFromAddress(uint16_t address) {
	return (uint16_t)(address - MODBUS_INPUT_REGISTERS_ADDRESS);
}

static uint16_t coilIndexFromAddress(uint16_t address) {
	return (uint16_t)(address - MODBUS_COILS_ADDRESS);
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

