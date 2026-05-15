/// @file modbusCustom.c
/// This file contains the implementation of callback functions for FreeModbus stack.
// This source code file was written by K.O. (2025 - 2026)

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "modbusConfig.h"
#include "debuggingTools.h"
#include "mb.h"
#include "sharedData.h"

static bool isDefinedCoilAddress(USHORT address) {
	return ((address >= MODBUS_ADDR_CUP1_CONTROL) && (address <= MODBUS_ADDR_CUP3_SWITCH2)) ||
	       ((address >= MODBUS_ADDR_CUP1_REQUESTED_STATE) && (address <= MODBUS_ADDR_ACTUATOR3_CONTROL));
}

static bool isWritableCoilAddress(USHORT address) {
	return (address == MODBUS_ADDR_CUP1_CONTROL) ||
	       (address == MODBUS_ADDR_CUP2_CONTROL) ||
	       (address == MODBUS_ADDR_CUP3_CONTROL);
}

static bool isDefinedHoldingAddress(USHORT address) {
	return (address >= MODBUS_ADDR_ERROR_CODE) && (address <= MODBUS_ADDR_CUP3_CH4_UPPER_LIMIT);
}

static bool isWritableHoldingAddress(USHORT address) {
	if ((address >= MODBUS_ADDR_ERROR_STORAGE) && (address <= MODBUS_ADDR_ACTIVE_CUP)){
		return true;
	}
	if ((address >= MODBUS_ADDR_CUP1_ERROR_STORAGE) && (address <= MODBUS_ADDR_CUP3_CH4_UPPER_LIMIT)){
		return true;
	}
	return false;
}

static bool isValidHoldingValue(USHORT address, uint16_t value) {
	if ((address >= MODBUS_ADDR_TIME_LIMIT_INSERTING_1) && (address <= MODBUS_ADDR_TIME_LIMIT_WITHDRAWING_3)) {
		return value >= 1u;
	}
	if (address == MODBUS_ADDR_ACTIVE_CUP) {
		return (value >= 1u) && (value <= 3u);
	}
	if (address == MODBUS_ADDR_INSTALLED_CUPS) {
		return (value >= 1u) && (value <= 3u);
	}
	if ((address >= MODBUS_ADDR_ELECTRODES_CUP1) && (address <= MODBUS_ADDR_ELECTRODES_CUP3)) {
		return (value >= 1u) && (value <= 4u);
	}
	if ((address >= MODBUS_ADDR_CUP1_TYPE) && (address <= MODBUS_ADDR_CUP3_TYPE)) {
		return value <= 1u;
	}

	return true;
}

static bool isDefinedInputRegisterAddress(USHORT address) {
	return (address >= MODBUS_ADDR_CUP1_CH1_SAMPLE) && (address <= MODBUS_ADDR_CUP3_CHANNEL_ERROR);
}

static uint16_t holdingIndexFromAddress(USHORT address) {
	return (uint16_t)(address - MODBUS_HOLDING_REGISTERS_ADDRESS);
}

static uint16_t inputIndexFromAddress(USHORT address) {
	return (uint16_t)(address - MODBUS_INPUT_REGISTERS_ADDRESS);
}

static uint16_t coilIndexFromAddress(USHORT address) {
	return (uint16_t)(address - MODBUS_COILS_ADDRESS);
}

/// This is callback function for reading and writing holding registers
/// @callgraph
/// @callergraph
eMBErrorCode    eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
                                 USHORT usNRegs, eMBRegisterMode eMode )
{
	if (usNRegs == 0u) {
		return MB_ENOREG;
	}

	if (MB_REG_WRITE == eMode) {
		for (USHORT k = 0; k < usNRegs; k++) {
			USHORT address = (USHORT)(usAddress + k);
			if (!isDefinedHoldingAddress(address)) {
				return MB_ENOREG;
			}

#if !MODBUS_CUSTOM_DEBUG_NO_LIMITS
			if (!isWritableHoldingAddress(address)) {
				return MB_ENOREG;
			}
#endif

			uint16_t value = ((uint16_t)pucRegBuffer[2u * k] << 8) | (uint16_t)pucRegBuffer[2u * k + 1u];

#if !MODBUS_CUSTOM_DEBUG_NO_LIMITS
			if (!isValidHoldingValue(address, value)) {
				return MB_EINVAL;
			}
#endif

			ModbusHoldingRegisters[holdingIndexFromAddress(address)] = value;
		}
        return MB_ENOERR;
	}

	for (USHORT k = 0; k < usNRegs; k++) {
		USHORT address = (USHORT)(usAddress + k);
		if (!isDefinedHoldingAddress(address)) {
			return MB_ENOREG;
		}

		uint16_t value = ModbusHoldingRegisters[holdingIndexFromAddress(address)];
		pucRegBuffer[2u * k] = (uint8_t)(value >> 8);
		pucRegBuffer[2u * k + 1u] = (uint8_t)(value & 0xFFu);
	}

	return MB_ENOERR;
}

/// This is callback function for reading and writing coils
/// @callgraph
/// @callergraph
eMBErrorCode    eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                               USHORT usNCoils, eMBRegisterMode eMode )
{
	if (usNCoils == 0u) {
		return MB_ENOREG;
	}

	if (MB_REG_WRITE == eMode) {
		if (usNCoils != 1u) {
			return MB_ENOREG;
		}

		if (!isDefinedCoilAddress(usAddress)) {
			return MB_ENOREG;
		}

#if !MODBUS_CUSTOM_DEBUG_NO_LIMITS
		if (!isWritableCoilAddress(usAddress)) {
			return MB_ENOREG;
		}
#endif

		uint16_t index = coilIndexFromAddress(usAddress);
		assert(index < MODBUS_COILS_NUMBER);

		bool value = (0u != pucRegBuffer[0]);
		ModbusCoils[index] = value;
		CoilsChanged[index] = true;

		return MB_ENOERR;
	}

	for (USHORT k = 0; k < usNCoils; k++) {
		USHORT address = (USHORT)(usAddress + k);
		if (!isDefinedCoilAddress(address)) {
			return MB_ENOREG;
		}

		if ((k % 8u) == 0u) {
			pucRegBuffer[k / 8u] = 0u;
		}

		uint16_t index = coilIndexFromAddress(address);
		assert(index < MODBUS_COILS_NUMBER);
		if (ModbusCoils[index]) {
			pucRegBuffer[k / 8u] |= (1u << (k % 8u));
		}
	}

	return MB_ENOERR;
}

/// This is callback function for reading and writing input registers
/// @callgraph
/// @callergraph
eMBErrorCode    eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress,
                               USHORT usNRegs )
{
	if (usNRegs == 0u) {
		return MB_ENOREG;
	}

	for (USHORT k = 0; k < usNRegs; k++) {
		USHORT address = (USHORT)(usAddress + k);
		if (!isDefinedInputRegisterAddress(address)) {
			return MB_ENOREG;
		}

		uint16_t value = ModbusInputRegisters[inputIndexFromAddress(address)];
		pucRegBuffer[2u * k] = (uint8_t)(value >> 8);
		pucRegBuffer[2u * k + 1u] = (uint8_t)(value & 0xFFu);
	}

	return MB_ENOERR;
}
