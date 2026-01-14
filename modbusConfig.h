/// @file modbusConfig.h
// This header file was written by K.O. (2025 - 2026)

#ifndef _MODBUS_CONFIG_H
#define _MODBUS_CONFIG_H

#include <stdatomic.h>
#include "hardware/uart.h"

// Which uart is used by Pico
#define MODBUS_UART_ID						uart0

// Which GPIO port is used as UART TX
#define MODBUS_UART_TX_PIN 					0

// Which GPIO port is used as UART RX
#define MODBUS_UART_RX_PIN 					1

// Parameters of Modbus transmission
#define MODBUS_BAUD_RATE 					19200
#define MODBUS_DATA_BITS 					8
#define MODBUS_PARITY    					UART_PARITY_EVEN

#define MODBUS_UART0_IRQ					UART0_IRQ

// The LED connected to GP5 flashes when the slave receives the Modbus frame
#define MODBUS_ACTIVITY_LED					5

// This directive specifies how long the LED should be lit, if any transmission from the master
// have been received; it should be coherent with timing of MainTimer
#define MODBUS_ACTIVITY_SHORT_TICKS			0x1500	// a bit less than 2 ms

// This directive specifies how long the LED should be lit, if the slave sends a response
// to the master; it should be coherent with timing of MainTimer
#define MODBUS_ACTIVITY_LONG_TICKS			0x9500ul	// a bit less than 10 ms

// This directive specifies the address of the slave
#define MODBUS_SLAVE_ID						1



#define MODBUS_INPUT_REGISTERS_ADDRESS		3000

#define MODBUS_INPUT_REGISTERS_NUMBER		15	//	5

#define MODBUS_COILS_ADDRESS				1

#define MODBUS_COILS_NUMBER					3	//	1

static_assert(MODBUS_COILS_NUMBER <= 8, "Error (static_assert)");

// This directive specifies the starting address of the register area
#define MODBUS_HOLDING_REGISTERS_ADDRESS	1000

// The initial registers are of type r/w; this directive specifies number of the r/w registers
#define MODBUS_HOLDING_REGISTERS_NUMBER		10

//static_assert(MODBUS_REGISTERS_TOTAL_NUMBER == MODBUS_AREA_RW_REGISTERS + MODBUS_AREA_RO_REGISTERS, "Error (static_assert)");


// This variable enables stopping the modbus state machine instead of executing the function
// 'assert' that was in the original freemodbus source code.
// That enables failover and debugging.
extern atomic_bool ModbusAssertionFailed;

// This is a flag indicating that the LED should flash due to transmission from the master
extern atomic_bool ModbusActiveLedShort;

// This is a flag indicating that the LED should shine longer, due to sending
// a response back to the master
extern volatile bool ModbusActiveLedLong;


#endif // _MODBUS_CONFIG_H
