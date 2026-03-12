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
#include "mainTimer.h"
#include "masterConfig.h"
#include "mb.h"
#include "modbusConfig.h"
#include "pico/stdlib.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// This directive tells that the LED on pico PCB is connected to GPIO25 port
#define PICO_ON_BOARD_LED_PIN 25

//..............................................................................
// Variables for Modbus communication
//..............................................................................

uint16_t ModbusInputRegisters[MODBUS_INPUT_REGISTERS_NUMBER];

bool ModbusCoils[MODBUS_COILS_NUMBER];

bool CoilsChanged[MODBUS_COILS_NUMBER];

// This is a table of Modbus registers;
uint16_t ModbusHoldingRegisters[MODBUS_HOLDING_REGISTERS_NUMBER];

// This variable enables stopping the modbus state machine instead of executing the function
// 'assert' that was in the original freemodbus source code.
// That enables failover and debugging.
atomic_bool ModbusAssertionFailed;

// This is a flag indicating that the LED should flash due to transmission from the master
atomic_bool ModbusActiveLedShort;

// This is a flag indicating that the LED should shine longer, due to sending
// a response back to the master
volatile bool ModbusActiveLedLong;

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

// This function initializes and turns on the LED on pico board.
void turnOnLedOnBoard(void);

// This function initializes the operation of the LED that indicates Modbus transmission.
void initModbusActivityLed(void);

// This function supports the LED that indicates Modbus transmission (provides
// the correct timing and strength of illumination).
void modbusActivityLedService(void);

//..............................................................................
// The main routine of the project
//..............................................................................

int main() {
	//...............
	// Initializations of variables, peripherals, etc.
	//...............

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

	// simulation of signal propagation from user request to insert/remove cup to feedback from limit switch
	atomic_store_explicit(&DebugCountdownPropagationFromCoilToSwitch1, 0, memory_order_release);
	atomic_store_explicit(&DebugCountdownPropagationFromCoilToSwitch2, 0, memory_order_release);
	atomic_store_explicit(&DebugCountdownPropagationFromCoilToSwitch3, 0, memory_order_release);
	atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch1, false, memory_order_release);
	atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch2, false, memory_order_release);
	atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch3, false, memory_order_release);
	ModbusHoldingRegisters[15] = 100;

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

	//...............
	// The main loop
	//...............

	while (1) {

		if (atomic_load_explicit(&SixtyFourMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&SixtyFourMillisecondsTimeTick, false, memory_order_release);

			static uint8_t TicksCounter;
			TicksCounter++;
			TicksCounter &= 15;

			debugTerminalCommandInterpreter(&ModbusHoldingRegisters[0], MODBUS_HOLDING_REGISTERS_NUMBER, 'a');
			for (int J = 0; J < MODBUS_INPUT_REGISTERS_NUMBER; J++) {
				ModbusInputRegisters[J] = ModbusHoldingRegisters[J];
			}
			if (ModbusHoldingRegisters[19] != 0) { // e.g. "t=1;"
				ModbusHoldingRegisters[19] = 0;
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

			// ModbusHoldingRegisters[16] contains coils 0,1,2;  e.g. "q=2;"
			// ModbusHoldingRegisters[17] contains coils 3,4,5;  e.g. "r=5;"
			// ModbusHoldingRegisters[18] contains coils 6,7,8;  e.g. "s=7;"
			assert(MODBUS_CUPS_NUMBER == 3);

			for (int J = 0; J < MODBUS_COILS_NUMBER; J++) {
				if (CoilsChanged[J]) { // if the coil state changed via Modbus
					printf("coil number %d value set to %c\r\n", J, ModbusCoils[J] ? '1' : '0');
					CoilsChanged[J] = false;

					if (ModbusCoils[J]) {
						ModbusHoldingRegisters[16 + J / (MODBUS_COILS_NUMBER / MODBUS_CUPS_NUMBER)] |=
						    (1 << (J % (MODBUS_COILS_NUMBER / MODBUS_CUPS_NUMBER)));
					}
					else {
						ModbusHoldingRegisters[16 + J / (MODBUS_COILS_NUMBER / MODBUS_CUPS_NUMBER)] &=
						    (0xFFFFu - (1 << (J % (MODBUS_COILS_NUMBER / MODBUS_CUPS_NUMBER))));
					}
				}
			}

			for (int J = 0; J < MODBUS_COILS_NUMBER; J++) {
				ModbusCoils[J] = false;
			}

			for (int J = 0; J < MODBUS_COILS_NUMBER / MODBUS_CUPS_NUMBER; J++) {
				uint16_t Mask = (1 << J);
				if (0 != (ModbusHoldingRegisters[16] & Mask)) {
					ModbusCoils[J] = true;
				}
				if (0 != (ModbusHoldingRegisters[17] & Mask)) {
					ModbusCoils[3 + J] = true;
				}
				if (0 != (ModbusHoldingRegisters[18] & Mask)) {
					ModbusCoils[6 + J] = true;
				}
			}
		}

		if (atomic_load_explicit(&TwoMillisecondsTimeTick, memory_order_acquire)) {
			atomic_store_explicit(&TwoMillisecondsTimeTick, false, memory_order_release);

			modbusActivityLedService();

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

		if (atomic_load_explicit(&DebugCompletedPropagationFromCoilToSwitch1, memory_order_acquire)) {
			atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch1, false, memory_order_release);
			ModbusCoils[2] = ModbusCoils[0];
			CoilsChanged[2] = true;
			printf("Coil->Switch(0->2)\r\n");
		}
		if (atomic_load_explicit(&DebugCompletedPropagationFromCoilToSwitch2, memory_order_acquire)) {
			atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch2, false, memory_order_release);
			ModbusCoils[5] = ModbusCoils[3];
			CoilsChanged[5] = true;
			printf("Coil->Switch(3->5)\r\n");
		}
		if (atomic_load_explicit(&DebugCompletedPropagationFromCoilToSwitch3, memory_order_acquire)) {
			atomic_store_explicit(&DebugCompletedPropagationFromCoilToSwitch3, false, memory_order_release);
			ModbusCoils[8] = ModbusCoils[6];
			CoilsChanged[8] = true;
			printf("Coil->Switch(6->8)\r\n");
		}
	} // The main loop
}

//..............................................................................
// Definitions of functions
//..............................................................................

// This function initializes and turns on the LED on pico board.
void turnOnLedOnBoard(void) {
	gpio_init(PICO_ON_BOARD_LED_PIN);
	gpio_set_dir(PICO_ON_BOARD_LED_PIN, GPIO_OUT);
	gpio_put(PICO_ON_BOARD_LED_PIN, true);
}

// This function initializes the operation of the LED that indicates Modbus transmission.
void initModbusActivityLed(void) {
	gpio_init(GPIO_MODBUS_ACTIVITY_LED);
	gpio_set_dir(GPIO_MODBUS_ACTIVITY_LED, GPIO_OUT);
	gpio_put(GPIO_MODBUS_ACTIVITY_LED, false);

	ModbusActiveLedIsOnShort = false;
	ModbusActiveLedIsOnLong = false;
	atomic_store_explicit(&ModbusActiveLedShort, false, memory_order_release);
	ModbusActiveLedLong = false;
}

// This function supports the LED that indicates Modbus transmission (provides
// the correct timing and strength of illumination).
void modbusActivityLedService(void) {
	static uint64_t ModbusActiveLedTime, CurrentTime;
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
