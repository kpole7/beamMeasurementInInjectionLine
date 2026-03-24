/// @file modbusCustom.c
/// This file contains the implementation of callback functions for FreeModbus stack.
// This source code file was written by K.O. (2025 - 2026)

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "modbusConfig.h"
#include "debuggingTools.h"
#include "mb.h"
#include "sharedData.h"

/// This is callback function for reading and writing holding registers
/// @callgraph
/// @callergraph
eMBErrorCode    eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
                                 USHORT usNRegs, eMBRegisterMode eMode )
{
	uint8_t K;

	if (usAddress < MODBUS_HOLDING_REGISTERS_ADDRESS){
		return(MB_ENOREG);
	}
	if(MB_REG_WRITE==eMode){ // write to holding registers
		if (usNRegs > 1){
#if MODBUS_DEBUG_MODE
	    	logAddEvent("Holding3",0xFFFFu);
#endif
			return(MB_ENOREG);
		}
		if (usAddress > MODBUS_HOLDING_REGISTERS_ADDRESS+MODBUS_HOLDING_REGISTERS_NUMBER-1){
#if MODBUS_DEBUG_MODE
			logAddEvent("Holding4",usAddress+usNRegs);
#endif
			return(MB_ENOREG);
		}
#if MODBUS_DEBUG_MODE
    	logAddEvent("Holding5",0xFFFFu);
    	IsChangeModbusWrite = false;
#endif

		ModbusHoldingRegisters[usAddress-MODBUS_HOLDING_REGISTERS_ADDRESS] = (uint16_t)pucRegBuffer[0]*256 + (uint16_t)pucRegBuffer[1];
	}
	else{ // read holding registers
		if (usNRegs > MODBUS_HOLDING_REGISTERS_NUMBER){
			return(MB_ENOREG);
		}
		if (usAddress+usNRegs > MODBUS_HOLDING_REGISTERS_ADDRESS+MODBUS_HOLDING_REGISTERS_NUMBER){
			return(MB_ENOREG);
		}
#if MODBUS_DEBUG_MODE
    	logAddEvent("Holding6",0xFFFFu);
#endif
		for(K=0;K<usNRegs;K++){
			pucRegBuffer[2*K]   = (uint8_t)(ModbusHoldingRegisters[K+usAddress-MODBUS_HOLDING_REGISTERS_ADDRESS] >> 8);
			pucRegBuffer[2*K+1] = (uint8_t)(ModbusHoldingRegisters[K+usAddress-MODBUS_HOLDING_REGISTERS_ADDRESS] & 0xFFu);
		}
	}
	return(MB_ENOERR);
}

/// This is callback function for reading and writing coils
/// @callgraph
/// @callergraph
eMBErrorCode    eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                               USHORT usNCoils, eMBRegisterMode eMode )
{
	if (usAddress < MODBUS_COILS_ADDRESS){
		return(MB_ENOREG);
	}
	if(MB_REG_WRITE==eMode){
		if (    (usAddress != MODBUS_RW_COIL_FOR_CUP_1) &&
				(usAddress != MODBUS_RW_COIL_FOR_CUP_2) &&
				(usAddress != MODBUS_RW_COIL_FOR_CUP_3))
		{
			return(MB_ENOREG);
		}
		if (1 != usNCoils){		// "Write single coil" command is implemented; "write multiple coils" command is not
			return(MB_ENOREG);
		}

		uint16_t TemporaryIndex = usAddress - MODBUS_COILS_ADDRESS;
		assert( TemporaryIndex < MODBUS_COILS_NUMBER );

		if (0 != pucRegBuffer[0]){
			ModbusCoils[TemporaryIndex] = true;
		}
		else{
			ModbusCoils[TemporaryIndex] = false;
		}
		CoilsChanged[TemporaryIndex] = true;


		// simulation of signal propagation from user request to insert/remove cup to feedback from limit switch
		if (0 == TemporaryIndex){
			atomic_store_explicit( &DebugCountdownPropagationFromCoilToSwitch1, ModbusHoldingRegisters[15], memory_order_release );
		}
		if (3 == TemporaryIndex){
			atomic_store_explicit( &DebugCountdownPropagationFromCoilToSwitch2, ModbusHoldingRegisters[15], memory_order_release );
		}
		if (6 == TemporaryIndex){
			atomic_store_explicit( &DebugCountdownPropagationFromCoilToSwitch3, ModbusHoldingRegisters[15], memory_order_release );
		}

	}
	else{ // read coils status
		if (usNCoils > MODBUS_COILS_NUMBER){
			return(MB_ENOREG);
		}
		if (usAddress+usNCoils > MODBUS_COILS_ADDRESS+MODBUS_COILS_NUMBER){
			return(MB_ENOREG);
		}

		uint16_t TemporaryIndex = usAddress - MODBUS_COILS_ADDRESS;

		for(uint16_t K = 0; K < usNCoils; K++){
			if ((K % 8) == 0){ // if the coil no. K is the first bit (LSB) in the byte
				pucRegBuffer[K / 8] = 0;
			}
			assert( TemporaryIndex < MODBUS_COILS_NUMBER );
			if (ModbusCoils[TemporaryIndex]){
				pucRegBuffer[K / 8] |= (1 << (K % 8));
			}
			TemporaryIndex++;
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
	uint8_t K;

	if (usAddress < MODBUS_INPUT_REGISTERS_ADDRESS){
		return(MB_ENOREG);
	}
	if (usNRegs > MODBUS_INPUT_REGISTERS_NUMBER){
		return(MB_ENOREG);
	}
	if (usAddress+usNRegs > MODBUS_INPUT_REGISTERS_ADDRESS+MODBUS_INPUT_REGISTERS_NUMBER){
		return(MB_ENOREG);
	}
#if MODBUS_DEBUG_MODE
	logAddEvent("Inputs",0xFFFFu);
#endif

	for(K=0;K<usNRegs;K++){
		pucRegBuffer[2*K]   = (uint8_t)(ModbusInputRegisters[K+usAddress-MODBUS_INPUT_REGISTERS_ADDRESS] >> 8);
		pucRegBuffer[2*K+1] = (uint8_t)(ModbusInputRegisters[K+usAddress-MODBUS_INPUT_REGISTERS_ADDRESS] & 0xFFu);
	}
	return(MB_ENOERR);
}
