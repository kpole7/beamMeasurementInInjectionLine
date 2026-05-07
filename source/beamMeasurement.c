/// @file beamMeasurement.c
/// This source code file was written by K.O. (2025 - 2026)
///
///
/// Abbreviations:
/// PSU = power supply unit
/// FSM = finite state machine
/// ISR = interrupt service routine

#include "adcInputs.h"
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

static uint16_t holdingIndexFromAddress(uint16_t address);

static uint16_t inputIndexFromAddress(uint16_t address);

static uint16_t coilIndexFromAddress(uint16_t address);

static uint16_t clampInstalledCups(uint16_t value);

static uint16_t clampActiveCup(uint16_t value);

static HighLevelCtrlState HighLevelState;
static bool IsHighLevelStateInitialized;

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

		if (atomic_load_explicit(&SixtyFourMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&SixtyFourMillisecondsTimeTick, false, memory_order_release);

			slowProcessesService();
		}

		if (atomic_load_explicit(&TwoMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&TwoMillisecondsTimeTick, false, memory_order_release);

			modbusActivityLedService();
			highLevelCtrlService();

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
	uint16_t InstalledCupsAddress = holdingIndexFromAddress(0x1013u);
	uint16_t ElectrodesInsideCup1Address = holdingIndexFromAddress(0x1014u);
	uint16_t ElectrodesInsideCup2Address = holdingIndexFromAddress(0x1015u);
	uint16_t ElectrodesInsideCup3Address = holdingIndexFromAddress(0x1016u);
	uint16_t Cup1TypeAddress = holdingIndexFromAddress(0x1017u);
	uint16_t Cup2TypeAddress = holdingIndexFromAddress(0x1018u);
	uint16_t Cup3TypeAddress = holdingIndexFromAddress(0x1019u);
	uint16_t ActiveCupAddress = holdingIndexFromAddress(0x1009u);

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
	ModbusCoils[coilIndexFromAddress(MODBUS_RW_COIL_FOR_CUP_1)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_RW_COIL_FOR_CUP_2)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_RW_COIL_FOR_CUP_3)] = true;
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
	uint16_t ErrorStorageIndex = holdingIndexFromAddress(0x1002u);
	uint16_t ActiveCupIndex = holdingIndexFromAddress(0x1009u);
	uint16_t ErrorCodeIndex = holdingIndexFromAddress(0x1000u);
	uint16_t LastErrorIndex = holdingIndexFromAddress(0x1001u);

	memset(&Inputs, 0, sizeof(Inputs));

	Inputs.installed_cups = clampInstalledCups(ModbusHoldingRegisters[holdingIndexFromAddress(0x1013u)]);
	Inputs.external_inhibition = ModbusCoils[coilIndexFromAddress(0x0006u)];
	Inputs.cup_control[0] = ModbusCoils[coilIndexFromAddress(0x0001u)];
	Inputs.cup_control[1] = ModbusCoils[coilIndexFromAddress(0x0005u)];
	Inputs.cup_control[2] = ModbusCoils[coilIndexFromAddress(0x0009u)];
	Inputs.cup_error[0] = ModbusHoldingRegisters[holdingIndexFromAddress(0x100Au)];
	Inputs.cup_error[1] = ModbusHoldingRegisters[holdingIndexFromAddress(0x100Bu)];
	Inputs.cup_error[2] = ModbusHoldingRegisters[holdingIndexFromAddress(0x100Cu)];
	Inputs.cup_steady[0] = ModbusCoils[coilIndexFromAddress(0x0013u)];
	Inputs.cup_steady[1] = ModbusCoils[coilIndexFromAddress(0x0014u)];
	Inputs.cup_steady[2] = ModbusCoils[coilIndexFromAddress(0x0015u)];
	Inputs.cup_inserted[0] = ModbusCoils[coilIndexFromAddress(0x0016u)];
	Inputs.cup_inserted[1] = ModbusCoils[coilIndexFromAddress(0x0017u)];
	Inputs.cup_inserted[2] = ModbusCoils[coilIndexFromAddress(0x0018u)];

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

	ModbusCoils[coilIndexFromAddress(0x0010u)] = Outputs.cup_requested_state[0];
	ModbusCoils[coilIndexFromAddress(0x0011u)] = Outputs.cup_requested_state[1];
	ModbusCoils[coilIndexFromAddress(0x0012u)] = Outputs.cup_requested_state[2];
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

