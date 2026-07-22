/// @file sharedData.h
// This header file was written by K.O. (2025 - 2026)

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "modbusConfig.h"

//..............................................................................
// Modbus Address Mnemonics from ModbusRegisters.ods
//..............................................................................

#include "modbusPrimitives.h"

//..............................................................................
// Constants related to Modbus registers
//..............................................................................

#define MODBUS_ADDR_THE_LAST_COIL 23
#define MODBUS_ADDR_THE_LAST_HOLDING_REGISTER 1159
#define MODBUS_ADDR_THE_LAST_INPUT_REGISTER 3015

#define MODBUS_COILS_ADDRESS                MODBUS_ADDR_CUP1_CONTROL

/// This is the number of coils (as defined by Modbus); some of them are read-only, while others are read-write
#define MODBUS_COILS_NUMBER                 (MODBUS_ADDR_THE_LAST_COIL - MODBUS_ADDR_CUP1_CONTROL + 1)


// This directive specifies the starting address of the register area
#define MODBUS_HOLDING_REGISTERS_ADDRESS    MODBUS_ADDR_ERROR_CODE

// The initial registers are of type r/w; this directive specifies number of the r/w registers
#define MODBUS_HOLDING_REGISTERS_NUMBER     (MODBUS_ADDR_THE_LAST_HOLDING_REGISTER - MODBUS_ADDR_ERROR_CODE + 1)

#define MODBUS_CALIBRATION_Y_REGISTERS_ADDRESS    MODBUS_ADDR_CALIBRATION_CURRENT1

#define MODBUS_CALIBRATION_X_REGISTERS_ADDRESS    MODBUS_ADDR_CUP1_CHANNEL1_GAIN_LOW_POINT1


#define MODBUS_INPUT_REGISTERS_ADDRESS      MODBUS_ADDR_CUP1_CHANNEL1_SAMPLE

/// This is the number of read-only input registers
#define MODBUS_INPUT_REGISTERS_NUMBER       (MODBUS_ADDR_THE_LAST_INPUT_REGISTER - MODBUS_ADDR_CUP1_CHANNEL1_SAMPLE + 1)

//..............................................................................
// Definitions of variables concerning Modbus communication
//..............................................................................

extern uint16_t ModbusInputRegisters[MODBUS_INPUT_REGISTERS_NUMBER];
extern bool ModbusCoils[MODBUS_COILS_NUMBER];
extern bool ModbusCoilTrigger[MODBUS_COILS_NUMBER];
extern uint16_t ModbusHoldingRegisters[MODBUS_HOLDING_REGISTERS_NUMBER];


uint16_t holdingIndexFromAddress(uint16_t address);

uint16_t inputIndexFromAddress(uint16_t address);

uint16_t coilIndexFromAddress(uint16_t address);

void initializeModbusRegisters(void);


#endif // SHARED_DATA_H
