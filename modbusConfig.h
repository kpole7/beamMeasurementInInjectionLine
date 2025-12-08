// This header file was written by K.O. (2024 - 2025)

#ifndef _MODBUS_CONFIG_H
#define _MODBUS_CONFIG_H

#include "hardware/uart.h"

// Which uart is used by Pico
#define MODBUS_UART_ID		uart0

// Which GPIO port is used as UART TX
#define MODBUS_UART_TX_PIN 	0

// Which GPIO port is used as UART RX
#define MODBUS_UART_RX_PIN 	1

// Parameters of Modbus transmission
#define MODBUS_BAUD_RATE 	19200
#define MODBUS_DATA_BITS 	8
#define MODBUS_PARITY    	UART_PARITY_EVEN

#define MODBUS_UART0_IRQ	UART0_IRQ

// The LED connected to GP5 flashes when the slave receives the Modbus frame
#define MODBUS_ACTIVITY_LED			5

// This directive specifies how long the LED should be lit, if any transmission from the master
// have been received; it should be a multiple of USER_INTERFACE_TIME_TICK
#define MODBUS_ACTIVITY_SHORT_TICKS	0x2000

// This directive specifies how long the LED should be lit, if the slave sends a response
// to the master; it should be a multiple of USER_INTERFACE_TIME_TICK
#define MODBUS_ACTIVITY_LONG_TICKS	0x10000ul

// This directive specifies the address of the slave
#define MODBUS_SLAVE_ID				1

// This directive specifies the starting address of the register area
#define MODBUS_AREA_BEGIN_ADDRESS	1000

// The initial registers are of type r/w; this directive specifies number of the r/w registers
#define MODBUS_AREA_RW_REGISTERS	2

// The consecutive registers are of type ro; this directive specifies number of the ro registers
#define MODBUS_AREA_RO_REGISTERS	12

// List of Modbus registers
#define MODBUS_REGISTER_SET_POWER	0
#define MODBUS_REGISTER_SET_CURRENT	1
#define MODBUS_REGISTER_ID			2
#define MODBUS_REGISTER_STATUS		3
#define MODBUS_REGISTER_I_MEAN		4
#define MODBUS_REGISTER_I_MEDIAN	5
#define MODBUS_REGISTER_I_MIXED		6
#define MODBUS_REGISTER_I_PEAK_PEAK	7
#define MODBUS_REGISTER_I_STD_DEV	8
#define MODBUS_REGISTER_V_MEAN		9
#define MODBUS_REGISTER_V_MEDIAN	10
#define MODBUS_REGISTER_V_MIXED		11
#define MODBUS_REGISTER_V_PEAK_PEAK	12
#define MODBUS_REGISTER_V_STD_DEV	13
#define MODBUS_REGISTERS_TOTAL_NUMBER	14

static_assert(MODBUS_REGISTERS_TOTAL_NUMBER == MODBUS_AREA_RW_REGISTERS + MODBUS_AREA_RO_REGISTERS, "Error (static_assert)");

// Masks for status register (MODBUS_REGISTER_STATUS)
#define MODBUS_STATUS_POWER_ON			1
#define MODBUS_STATUS_LOCAL_REMOTE		2
#define MODBUS_STATUS_EXTERNAL_ERROR	4
#define MODBUS_STATUS_SUM_ERROR			8

// This variable enables stopping the modbus state machine instead of executing the function
// 'assert' that was in the original freemodbus source code.
// That enables failover and debugging.
extern volatile bool ModbusAssertionFailed;

// This is a flag indicating that the LED should flash due to transmission from the master
extern volatile bool ModbusActiveLedShort;

// This is a flag indicating that the LED should shine longer, due to sending
// a response back to the master
extern volatile bool ModbusActiveLedLong;


#endif // _MODBUS_CONFIG_H
