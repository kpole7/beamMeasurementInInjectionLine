// This source code file was written by K.O. (2024 - 2025)

#include <inttypes.h>
#include <stdbool.h>
#include "../modbusConfig.h"
#include "../debuggingTools.h"
#include "../spiCommunication.h"
#include "mb.h"

// This a table of Modbus registers; initial registers are of type r/w,
// subsequent registers are of type ro.
extern uint16_t ModbusRegisters[MODBUS_AREA_RW_REGISTERS+MODBUS_AREA_RO_REGISTERS];

// This flag indicates that the Modbus state machine has written a new value to a register
// so writing to the power source should be activated by the SPI.
extern bool IsWriteRequest;

// This is callback function for reading and writing registers
eMBErrorCode    eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
                                 USHORT usNRegs, eMBRegisterMode eMode )
{
	uint8_t K;

	if (usAddress < MODBUS_AREA_BEGIN_ADDRESS){
		return(MB_ENOREG);
	}
	if(MB_REG_WRITE==eMode){
		if (usNRegs > 1){
#if MODBUS_DEBUG_MODE
	    	logAddEvent("Holding3",0xFFFFu);
#endif
			return(MB_ENOREG);
		}
		if (usAddress > MODBUS_AREA_BEGIN_ADDRESS+MODBUS_AREA_RW_REGISTERS-1){
#if MODBUS_DEBUG_MODE
			logAddEvent("Holding4",usAddress+usNRegs);
#endif
			return(MB_ENOREG);
		}
#if MODBUS_DEBUG_MODE
    	logAddEvent("Holding5",0xFFFFu);
    	IsChangeModbusWrite = false;
#endif

		if(ModbusRegisters[usAddress-MODBUS_AREA_BEGIN_ADDRESS] != (uint16_t)pucRegBuffer[0]*256 + (uint16_t)pucRegBuffer[1]){
			if(MODBUS_AREA_BEGIN_ADDRESS+MODBUS_REGISTER_SET_POWER == usAddress){
				// This is a special case, as described in the K.O. documentation (section 2.5)
				// Any change of Power On/Off state implies zeroing setting of output current value
				ModbusRegisters[MODBUS_REGISTER_SET_CURRENT] = 0;
			}
#if MODBUS_DEBUG_MODE
			IsChangeModbusWrite = true;
#endif
		}
		ModbusRegisters[usAddress-MODBUS_AREA_BEGIN_ADDRESS] = (uint16_t)pucRegBuffer[0]*256 + (uint16_t)pucRegBuffer[1];
		IsWriteRequest = true; // This SPI write request is caused by the Modbus command
	}
	else{
		if (usNRegs > MODBUS_AREA_RW_REGISTERS+MODBUS_AREA_RO_REGISTERS){
			return(MB_ENOREG);
		}
		if (usAddress+usNRegs > MODBUS_AREA_BEGIN_ADDRESS+MODBUS_AREA_RW_REGISTERS+MODBUS_AREA_RO_REGISTERS){
			return(MB_ENOREG);
		}
#if MODBUS_DEBUG_MODE
    	logAddEvent("Stat->",0xFFFFu);
#endif
		calculateStatistics();
#if MODBUS_DEBUG_MODE
    	logAddEvent("<-Stat",0xFFFFu);
#endif
		for(K=0;K<usNRegs;K++){
			pucRegBuffer[2*K]   = (uint8_t)(ModbusRegisters[K+usAddress-MODBUS_AREA_BEGIN_ADDRESS] >> 8);
			pucRegBuffer[2*K+1] = (uint8_t)(ModbusRegisters[K+usAddress-MODBUS_AREA_BEGIN_ADDRESS] & 0xFFu);
		}
	}
	return(MB_ENOERR);
}

eMBErrorCode    eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                               USHORT usNCoils, eMBRegisterMode eMode )
{
	return MB_ENOREG;
}

eMBErrorCode    eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress,
                               USHORT usNRegs )
{
	uint8_t K;

	if (usAddress < MODBUS_AREA_INPUTS_ADDRESS){
		return(MB_ENOREG);
	}
	if (usNRegs > MODBUS_AREA_RO_INPUTS){
		return(MB_ENOREG);
	}
	if (usAddress+usNRegs > MODBUS_AREA_INPUTS_ADDRESS+MODBUS_AREA_RO_INPUTS){
		return(MB_ENOREG);
	}
#if MODBUS_DEBUG_MODE
	logAddEvent("Inputs",0xFFFFu);
#endif

	calculateStatistics(); // randomness in simulation
	for(K=0;K<usNRegs;K++){
		uint32_t TemporaryValue = (uint32_t)ModbusRegisters[MODBUS_REGISTER_I_MEAN];
		TemporaryValue *= 1000;
		TemporaryValue /= 307;
		TemporaryValue += ((usAddress - MODBUS_AREA_INPUTS_ADDRESS) + K) * 100;
		pucRegBuffer[2*K]   = (uint8_t)(TemporaryValue >> 8);
		pucRegBuffer[2*K+1] = (uint8_t)(TemporaryValue & 0xFFu);
	}
	return(MB_ENOERR);
}
