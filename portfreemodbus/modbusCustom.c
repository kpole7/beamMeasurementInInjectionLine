/// @file modbusCustom.c
// This source code file was written by K.O. (2025 - 2026)

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "../modbusConfig.h"
#include "../debuggingTools.h"
#include "../spiCommunication.h"
#include "mb.h"

extern uint16_t ModbusInputRegisters[MODBUS_INPUT_REGISTERS_NUMBER];

extern bool ModbusCoils[MODBUS_COILS_NUMBER];

extern bool CoilsChanged[MODBUS_COILS_NUMBER];

// This is a table of Modbus registers
extern uint16_t ModbusHoldingRegisters[MODBUS_HOLDING_REGISTERS_NUMBER];

/// This is callback function for reading and writing registers
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




		// for debugging purpose

		printf("Inputs= ");

		static_assert(MODBUS_INPUT_REGISTERS_NUMBER <= MODBUS_HOLDING_REGISTERS_NUMBER, "Error (static_assert)");

		for(K=0;K<MODBUS_INPUT_REGISTERS_NUMBER;K++){
			ModbusInputRegisters[K] = ModbusHoldingRegisters[K];
			printf(" %04X", (unsigned)ModbusInputRegisters[K]);
		}

		printf("\n");



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

static_assert(MODBUS_COILS_NUMBER <= 8, "Error (static_assert)");

/// @callgraph
/// @callergraph
eMBErrorCode    eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                               USHORT usNCoils, eMBRegisterMode eMode )
{
	if (usAddress < MODBUS_COILS_ADDRESS){
		return(MB_ENOREG);
	}

#if MODBUS_DEBUG_MODE
	logAddEvent("Inputs",0xFFFFu);
#endif

	if(MB_REG_WRITE==eMode){
		if (usAddress+1 > MODBUS_COILS_ADDRESS+MODBUS_COILS_NUMBER){
			return(MB_ENOREG);
		}

		bool TemporaryValue;
		if ((0xFF == pucRegBuffer[0]) && (0 == pucRegBuffer[1])){
			TemporaryValue = true;
		}
		else if ((0 == pucRegBuffer[0]) && (0 == pucRegBuffer[1])){
			TemporaryValue = false;
		}
		else{
			return(MB_EINVAL);
		}

		uint16_t TemporaryIndex = usAddress - MODBUS_COILS_ADDRESS;
		assert( TemporaryIndex < MODBUS_COILS_NUMBER );

		ModbusCoils[TemporaryIndex] = TemporaryValue;
		CoilsChanged[TemporaryIndex] = true;
	}
	else{ // read coils status
		if (usNCoils > MODBUS_COILS_NUMBER){
			return(MB_ENOREG);
		}
		if (usAddress+usNCoils > MODBUS_COILS_ADDRESS+MODBUS_COILS_NUMBER){
			return(MB_ENOREG);
		}

		uint16_t TemporaryValue = 0;
		uint16_t TemporaryIndex = usAddress - MODBUS_COILS_ADDRESS;

		for(uint16_t K = 0; K < usNCoils; K++){
			assert( TemporaryIndex < MODBUS_COILS_NUMBER );
			if (ModbusCoils[TemporaryIndex]){
				TemporaryValue |= (1 << K);
			}
			TemporaryIndex++;
		}
		pucRegBuffer[0] = (UCHAR)TemporaryValue;
	}

	return MB_ENOREG;
}

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
