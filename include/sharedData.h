/// @file sharedData.h
// This header file was written by K.O. (2025 - 2026)

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "modbusConfig.h"

//..............................................................................
// Definitions of variables concerning Modbus communication
//..............................................................................

extern uint16_t ModbusInputRegisters[MODBUS_INPUT_REGISTERS_NUMBER];
extern bool ModbusCoils[MODBUS_COILS_NUMBER];
extern bool CoilsChanged[MODBUS_COILS_NUMBER];
extern uint16_t ModbusHoldingRegisters[MODBUS_HOLDING_REGISTERS_NUMBER];

#endif // SHARED_DATA_H