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
	return ((address >= 0x0001u) && (address <= 0x000Cu)) ||
	       ((address >= 0x0100u) && (address <= 0x0115u));
}

static bool isWritableCoilAddress(USHORT address) {
	return (address == MODBUS_RW_COIL_FOR_CUP_1) ||
	       (address == MODBUS_RW_COIL_FOR_CUP_2) ||
	       (address == MODBUS_RW_COIL_FOR_CUP_3);
}

static bool isDefinedHoldingAddress(USHORT address) {
	return ((address >= 0x1000u) && (address <= 0x1002u)) ||
	       ((address >= 0x1010u) && (address <= 0x1015u)) ||
	       (address == 0x1020u) ||
	       ((address >= 0x1030u) && (address <= 0x1038u)) ||
	       ((address >= 0x1100u) && (address <= 0x1106u)) ||
	       ((address >= 0x1200u) && (address <= 0x1217u)) ||
	       ((address >= 0x1240u) && (address <= 0x1257u)) ||
	       (address == 0x1280u) ||
	       ((address >= 0x12A0u) && (address <= 0x12ABu)) ||
	       ((address >= 0x12B0u) && (address <= 0x12BBu));
}

static bool isWritableHoldingAddress(USHORT address) {
	return (address == 0x1002u) ||
	       ((address >= 0x1010u) && (address <= 0x1015u)) ||
	       (address == 0x1020u) ||
	       ((address >= 0x1036u) && (address <= 0x1038u)) ||
	       ((address >= 0x1100u) && (address <= 0x1106u)) ||
	       ((address >= 0x1200u) && (address <= 0x1217u)) ||
	       ((address >= 0x1240u) && (address <= 0x1257u)) ||
	       (address == 0x1280u) ||
	       ((address >= 0x12A0u) && (address <= 0x12ABu)) ||
	       ((address >= 0x12B0u) && (address <= 0x12BBu));
}

static bool isValidHoldingValue(USHORT address, uint16_t value) {
	if ((address >= 0x1010u) && (address <= 0x1015u)) {
		return value >= 1u;
	}
	if (address == 0x1020u) {
		return (value >= 1u) && (value <= 3u);
	}
	if (address == 0x1100u) {
		return (value >= 1u) && (value <= 3u);
	}
	if ((address >= 0x1101u) && (address <= 0x1103u)) {
		return (value >= 1u) && (value <= 4u);
	}
	if ((address >= 0x1104u) && (address <= 0x1106u)) {
		return value <= 1u;
	}

	return true;
}

static bool isDefinedInputRegisterAddress(USHORT address) {
	return ((address >= 0x3001u) && (address <= 0x3005u)) ||
	       ((address >= 0x3011u) && (address <= 0x3015u)) ||
	       ((address >= 0x3021u) && (address <= 0x3025u));
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

		if (usAddress == MODBUS_RW_COIL_FOR_CUP_1) {
			atomic_store_explicit(&DebugCountdownPropagationFromCoilToSwitch1,
			                      ModbusHoldingRegisters[holdingIndexFromAddress(0x1010u)],
			                      memory_order_release);
		}
		if (usAddress == MODBUS_RW_COIL_FOR_CUP_2) {
			atomic_store_explicit(&DebugCountdownPropagationFromCoilToSwitch2,
			                      ModbusHoldingRegisters[holdingIndexFromAddress(0x1011u)],
			                      memory_order_release);
		}
		if (usAddress == MODBUS_RW_COIL_FOR_CUP_3) {
			atomic_store_explicit(&DebugCountdownPropagationFromCoilToSwitch3,
			                      ModbusHoldingRegisters[holdingIndexFromAddress(0x1012u)],
			                      memory_order_release);
		}

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
