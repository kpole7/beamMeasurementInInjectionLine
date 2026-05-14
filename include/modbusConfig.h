/// @file modbusConfig.h
// This header file was written by K.O. (2025 - 2026)

#ifndef MODBUS_CONFIG_H
#define MODBUS_CONFIG_H

#include "hardware/uart.h"
#include <stdatomic.h>

// Which uart is used by Pico
#define MODBUS_UART_ID uart0

// Which GPIO port is used as UART TX
#define MODBUS_UART_TX_PIN 0

// Which GPIO port is used as UART RX
#define MODBUS_UART_RX_PIN 1

// Parameters of Modbus transmission
#define MODBUS_BAUD_RATE 19200
#define MODBUS_DATA_BITS 8
#define MODBUS_PARITY UART_PARITY_EVEN

#define MODBUS_UART0_IRQ UART0_IRQ

// The LED connected to GP5 flashes when the slave receives the Modbus frame
#define GPIO_MODBUS_ACTIVITY_LED 5

// This directive specifies how long the LED should be lit, if any transmission from the master
// have been received; it should be coherent with timing of MainTimer
#define MODBUS_ACTIVITY_SHORT_TICKS 0x1500 // a bit less than 2 ms

// This directive specifies how long the LED should be lit, if the slave sends a response
// to the master; it should be coherent with timing of MainTimer
#define MODBUS_ACTIVITY_LONG_TICKS 0x9500ul // a bit less than 10 ms

// This directive specifies the address of the slave
#define MODBUS_SLAVE_ID 1

// Mode selection for modbusCustom.c:
// 0 = normal mode (read/write restrictions and value checks enabled)
// 1 = debug mode (all defined coils and holding registers are writable, no value checks)
#define MODBUS_CUSTOM_DEBUG_NO_LIMITS 0

// This variable enables stopping the modbus state machine instead of executing the function
// 'assert' that was in the original freemodbus source code.
// That enables failover and debugging.
extern atomic_bool ModbusAssertionFailed;

// This is a flag indicating that the LED should flash due to transmission from the master
extern atomic_bool ModbusActiveLedShort;

// This is a flag indicating that the LED should shine longer, due to sending
// a response back to the master
extern volatile bool ModbusActiveLedLong;

#endif // MODBUS_CONFIG_H
