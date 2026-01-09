/// @file beamMeasurement.c
/// This source code file was written by K.O. (2025 - 2026)
///
///
/// Abbreviations:
/// PSU = power supply unit
/// FSM = finite state machine
/// ISR = interrupt service routine



#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "pico/stdlib.h"
#include "modbusConfig.h"
#include "masterConfig.h"
#include "mainTimer.h"
#include "adcInputs.h"
#include "debuggingTools.h"
#include "mb.h"


// This directive tells that the LED on pico PCB is connected to GPIO25 port
#define PICO_ON_BOARD_LED_PIN		25

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


int main(){
	uint8_t J;

	//...............
	// Initializations of variables, peripherals, etc.
	//...............

	atomic_store_explicit( &ModbusAssertionFailed, false, memory_order_release );

	stdio_init_all();
	turnOnLedOnBoard();
	initModbusActivityLed();
	initializeAdcMeasurements();

	for (J=0; J<MODBUS_HOLDING_REGISTERS_NUMBER;J++){
		ModbusHoldingRegisters[J] = 0;
	}
	for (J=0; J<MODBUS_COILS_NUMBER; J++){
		ModbusCoils[J] = false;
		CoilsChanged[J] = false;
	}

#if MODBUS_DEBUG_MODE
	initInputPortJP1();
	initAuxiliaryPrintouts();
#endif
	initInputPortJP2();
	printf("\r\nHello, world!\r\n");

	auxiliaryOutputsInitialize();

	eMBInit(MB_RTU,MODBUS_SLAVE_ID,0,MODBUS_BAUD_RATE,MODBUS_PARITY); // The parameter ucPort is a dummy and will be ignored
	eMBEnable();

	startPeriodicInterrupt();

	//...............
	// The main loop
	//...............

    while (1){

    	if(atomic_load_explicit( &SixtyFourMillisecondsTimeTick, memory_order_acquire )){
    		atomic_store_explicit( &SixtyFourMillisecondsTimeTick, false, memory_order_release );

    		static uint8_t TicksCounter;
    		TicksCounter++;
    		TicksCounter &= 15;

    		debugTerminalCommandInterpreter( &ModbusInputRegisters[0], MODBUS_INPUT_REGISTERS_NUMBER, 'a' );


#if 1	// just for debugging
    		for (J = 0; J < MODBUS_COILS_NUMBER; J++){
    			if (CoilsChanged[J]){
    				printf("coil number %d value set to %c\r\n", J, ModbusCoils[J]? '1' : '0' );
    				CoilsChanged[J] = false;
    			}
    		}
#endif


    	}

    	if(atomic_load_explicit( &TwoMillisecondsTimeTick, memory_order_acquire )){
    		atomic_store_explicit( &TwoMillisecondsTimeTick, false, memory_order_release );

    		modbusActivityLedService();

#if MODBUS_DEBUG_MODE
    		// Reading the states of jumpers.
    		IsJumperJP1 = !readInputPortJP1(); // false;	// Modbus state machine debugging
#endif
    	}

#if MODBUS_DEBUG_MODE
    	// Auxiliary printouts for debugging purpose
    	if(IsJumperJP1 && !OldIsJumperJP1){
    		logPrintAll(0);
    	}
    	OldIsJumperJP1 = IsJumperJP1;
#endif

    	// Modbus communication service
    	if(!atomic_load_explicit( &ModbusAssertionFailed, memory_order_acquire )){

    		eMBPoll();

    	}
    	else{
    		// Stopping the modbus state machine instead of executing the function 'assert'
    		// that was in the original freemodbus source code.
    		// That enables failover and debugger actions.
    		if(irq_is_enabled(MODBUS_UART0_IRQ)){
    		    irq_set_enabled(MODBUS_UART0_IRQ, false);
    		}
    	}
    } // The main loop
}

//..............................................................................
// Definitions of functions
//..............................................................................


// This function initializes and turns on the LED on pico board.
void turnOnLedOnBoard(void){
	gpio_init(PICO_ON_BOARD_LED_PIN);
	gpio_set_dir(PICO_ON_BOARD_LED_PIN, GPIO_OUT);
	gpio_put(PICO_ON_BOARD_LED_PIN, true);
}

// This function initializes the operation of the LED that indicates Modbus transmission.
void initModbusActivityLed(void){
	gpio_init(MODBUS_ACTIVITY_LED);
	gpio_set_dir(MODBUS_ACTIVITY_LED, GPIO_OUT);
	gpio_put(MODBUS_ACTIVITY_LED, false);

	ModbusActiveLedIsOnShort=false;
	ModbusActiveLedIsOnLong=false;
	atomic_store_explicit( &ModbusActiveLedShort, false, memory_order_release );
	ModbusActiveLedLong=false;
}

// This function supports the LED that indicates Modbus transmission (provides
// the correct timing and strength of illumination).
void modbusActivityLedService(void){
	static uint64_t ModbusActiveLedTime, CurrentTime;
	CurrentTime = time_us_64();
	if(atomic_load_explicit( &ModbusActiveLedShort, memory_order_acquire ) && !ModbusActiveLedIsOnShort){
		ModbusActiveLedIsOnShort=true;
		gpio_put(MODBUS_ACTIVITY_LED, true);
		gpio_set_drive_strength(MODBUS_ACTIVITY_LED,GPIO_DRIVE_STRENGTH_2MA);
		ModbusActiveLedTime = CurrentTime + (uint64_t)MODBUS_ACTIVITY_SHORT_TICKS;
	}
	if(ModbusActiveLedLong && !ModbusActiveLedIsOnLong){
		ModbusActiveLedIsOnLong=true;
		gpio_put(MODBUS_ACTIVITY_LED, true);
		gpio_set_drive_strength(MODBUS_ACTIVITY_LED,GPIO_DRIVE_STRENGTH_12MA);
		ModbusActiveLedTime = CurrentTime + (uint64_t)MODBUS_ACTIVITY_LONG_TICKS;
	}
	if(CurrentTime > ModbusActiveLedTime){
		gpio_put(MODBUS_ACTIVITY_LED, false);
		ModbusActiveLedIsOnShort=false;
		ModbusActiveLedIsOnLong=false;
		atomic_store_explicit( &ModbusActiveLedShort, false, memory_order_release );
		ModbusActiveLedLong=false;
	}
}
