/// @file sharedData.c
// This source code file was written by K.O. (2025 - 2026)

#include "sharedData.h"
#include "mainTimer.h"
#include "compilationTime.h"

#include <stdlib.h> // rand()

//..............................................................................
// Variables for Modbus communication
//..............................................................................

/// This is a table of Modbus input registers to monitor the state of the hardware.
uint16_t ModbusInputRegisters[MODBUS_INPUT_REGISTERS_NUMBER];

/// This is a table of Modbus coils to control the hardware. Some of them are read-only, some are read-write.
bool ModbusCoils[MODBUS_COILS_NUMBER];

/// This is a table indicating which coils have changed via Modbus communication.
bool ModbusCoilTrigger[MODBUS_COILS_NUMBER];

// This is a table of Modbus registers for debugging purposes.
uint16_t ModbusHoldingRegisters[MODBUS_HOLDING_REGISTERS_NUMBER];

/// This variable enables stopping the modbus state machine instead of executing the function
/// 'assert' that was in the original freemodbus source code. That enables failover and debugging.
atomic_bool ModbusAssertionFailed;

/// This is a flag indicating that the LED should flash due to transmission from the master
atomic_bool ModbusActiveLedShort;

/// This is a flag indicating that the LED should shine longer, due to sending a response back to the master.
volatile bool ModbusActiveLedLong;

uint16_t holdingIndexFromAddress(uint16_t address) {
	return (uint16_t)(address - MODBUS_HOLDING_REGISTERS_ADDRESS);
}

uint16_t inputIndexFromAddress(uint16_t address) {
	return (uint16_t)(address - MODBUS_INPUT_REGISTERS_ADDRESS);
}

uint16_t coilIndexFromAddress(uint16_t address) {
	return (uint16_t)(address - MODBUS_COILS_ADDRESS);
}

void initializeModbusRegisters(void) {
	for (int J = 0; J < MODBUS_HOLDING_REGISTERS_NUMBER; J++) {
		ModbusHoldingRegisters[J] = 0;
	}
	for (int J = 0; J < MODBUS_COILS_NUMBER; J++) {
		ModbusCoils[J] = false;
		ModbusCoilTrigger[J] = false;
	}

	// Defaults from ModbusRegisters.csv
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_CONTROL)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_CONTROL)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_CONTROL)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR1_CONTROL)] = true;
	ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_ACTUATOR2_CONTROL)] = true;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING1)] = 700u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING2)] = 700u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_INSERTING3)] = 4000u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING1)] = 300u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING2)] = 300u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_TIME_LIMIT_WITHDRAWING3)] = 4400u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_INSTALLED_CUPS)] = 3u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_ELECTRODES_CUP1)] = 4u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_ELECTRODES_CUP2)] = 4u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_ELECTRODES_CUP3)] = 4u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_TYPE)] = 1u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_TYPE)] = 2u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CALIBRATION_CURRENT1)] = 24169u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CALIBRATION_CURRENT2)] = 2410u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CALIBRATION_CURRENT3)] = 2198u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CALIBRATION_CURRENT4)] = 158u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL1_GAIN_LOW_POINT1)] = 1725u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL1_GAIN_LOW_POINT2)] = 180u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL1_GAIN_LOW_POINT3)] = 164u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL1_GAIN_HIGH_POINT2)] = 1741u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL1_GAIN_HIGH_POINT3)] = 1589u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL1_GAIN_HIGH_POINT4)] = 43u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL2_GAIN_LOW_POINT1)] = 1732u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL2_GAIN_LOW_POINT2)] = 184u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL2_GAIN_LOW_POINT3)] = 166u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL2_GAIN_HIGH_POINT2)] = 1766u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL2_GAIN_HIGH_POINT3)] = 1622u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL2_GAIN_HIGH_POINT4)] = 66u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL3_GAIN_LOW_POINT1)] = 1731u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL3_GAIN_LOW_POINT2)] = 187u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL3_GAIN_LOW_POINT3)] = 171u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL3_GAIN_HIGH_POINT2)] = 1795u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL3_GAIN_HIGH_POINT3)] = 1661u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL3_GAIN_HIGH_POINT4)] = 108u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL4_GAIN_LOW_POINT1)] = 1722u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL4_GAIN_LOW_POINT2)] = 177u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL4_GAIN_LOW_POINT3)] = 162u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL4_GAIN_HIGH_POINT2)] = 1719u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL4_GAIN_HIGH_POINT3)] = 1566u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP1_CHANNEL4_GAIN_HIGH_POINT4)] = 22u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL1_GAIN_LOW_POINT1)] = 1727u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL1_GAIN_LOW_POINT2)] = 185u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL1_GAIN_LOW_POINT3)] = 167u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL1_GAIN_HIGH_POINT2)] = 1770u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL1_GAIN_HIGH_POINT3)] = 1625u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL1_GAIN_HIGH_POINT4)] = 75u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL2_GAIN_LOW_POINT1)] = 1728u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL2_GAIN_LOW_POINT2)] = 185u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL2_GAIN_LOW_POINT3)] = 169u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL2_GAIN_HIGH_POINT2)] = 1774u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL2_GAIN_HIGH_POINT3)] = 1632u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL2_GAIN_HIGH_POINT4)] = 81u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL3_GAIN_LOW_POINT1)] = 1718u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL3_GAIN_LOW_POINT2)] = 176u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL3_GAIN_LOW_POINT3)] = 163u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL3_GAIN_HIGH_POINT2)] = 1713u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL3_GAIN_HIGH_POINT3)] = 1558u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL3_GAIN_HIGH_POINT4)] = 22u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL4_GAIN_LOW_POINT1)] = 1741u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL4_GAIN_LOW_POINT2)] = 198u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL4_GAIN_LOW_POINT3)] = 182u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL4_GAIN_HIGH_POINT2)] = 1851u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL4_GAIN_HIGH_POINT3)] = 1752u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP2_CHANNEL4_GAIN_HIGH_POINT4)] = 213u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL1_GAIN_LOW_POINT1)] = 1725u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL1_GAIN_LOW_POINT2)] = 181u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL1_GAIN_LOW_POINT3)] = 165u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL1_GAIN_HIGH_POINT2)] = 1751u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL1_GAIN_HIGH_POINT3)] = 1603u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL1_GAIN_HIGH_POINT4)] = 55u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL2_GAIN_LOW_POINT1)] = 1724u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL2_GAIN_LOW_POINT2)] = 181u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL2_GAIN_LOW_POINT3)] = 166u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL2_GAIN_HIGH_POINT2)] = 1756u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL2_GAIN_HIGH_POINT3)] = 1608u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL2_GAIN_HIGH_POINT4)] = 59u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL3_GAIN_LOW_POINT1)] = 1724u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL3_GAIN_LOW_POINT2)] = 180u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL3_GAIN_LOW_POINT3)] = 165u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL3_GAIN_HIGH_POINT2)] = 1746u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL3_GAIN_HIGH_POINT3)] = 1595u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL3_GAIN_HIGH_POINT4)] = 49u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL4_GAIN_LOW_POINT1)] = 1724u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL4_GAIN_LOW_POINT2)] = 181u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL4_GAIN_LOW_POINT3)] = 164u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL4_GAIN_HIGH_POINT2)] = 1744u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL4_GAIN_HIGH_POINT3)] = 1593u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_CUP3_CHANNEL4_GAIN_HIGH_POINT4)] = 44u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME01)] = 19317u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME02)] = 25195u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME03)] = 26912u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME04)] = 18017u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME05)] = 29281u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME06)] = 25697u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEVICE_NAME07)] = 31073u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM_PROPAGATION1_IN)] = 75u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM_PROPAGATION1_OUT)] = 70u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM_PROPAGATION2_IN)] = 405u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM_PROPAGATION2_OUT)] = 100u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM_PROPAGATION3_IN)] = 1370u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM_PROPAGATION3_OUT)] = 1410u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM3_INERTIAL_MOTION)] = 210u;
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_MECHANISM3_BRAKED_MOTION)] = 35u;

	for (int J = 0; J <= MODBUS_ADDR_COMPILATION_TIME11 - MODBUS_ADDR_COMPILATION_TIME01; J++) {
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_COMPILATION_TIME01) + J] = 	
			((uint16_t)CompilationTime[J*2] << 8) + (uint16_t)CompilationTime[J*2 + 1];
	}

	// filling with random values for testing purposes
	for (int J = 0; J <= MODBUS_ADDR_SIM_AMPLIFIER_CUP3_ELECTRODE4 - MODBUS_ADDR_SIM_AMPLIFIER_CUP1_ELECTRODE1; J++) {
		ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_AMPLIFIER_CUP1_ELECTRODE1) + J] = 
			(uint16_t)((rand() % 155u)*(rand() % 155u)); // 155*155=24025 / 100 = 240.25 uA
	}
	// live random values for testing purposes
	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_SIM_AMPLIFIER_RANDOM_OFFSET)] = 20u;

	ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] = 6u; // PRINTOUTS_LOGIC | PRINTOUTS_ACTUATORS
}


