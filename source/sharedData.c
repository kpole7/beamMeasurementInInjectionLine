/// @file sharedData.c
// This source code file was written by K.O. (2025 - 2026)

#include "sharedData.h"

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

