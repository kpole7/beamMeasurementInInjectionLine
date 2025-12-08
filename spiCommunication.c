// This source code file was written by K.O. (2024 - 2025)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "masterConfig.h"
#include "modbusConfig.h"
#include "spiCommunication.h"
#include "debuggingTools.h"

//..............................................................................
// External variables
//..............................................................................


// This a table of Modbus registers; initial registers are of type r/w,
// subsequent registers are of type ro.
extern uint16_t ModbusRegisters[MODBUS_AREA_RW_REGISTERS+MODBUS_AREA_RO_REGISTERS];

#if MODBUS_DEBUG_MODE
extern bool IsJumperJP2;
#endif


//..............................................................................
// Definitions of constants
//..............................................................................


// SPI hardware clock frequency
#define SPI_CLOCK_FREQUENCY				1000000ul

// The number of registers supported by SPI
#define SPI_BUFFER_LENGTH				6

// The line 'LD' of the power source is connected to GPIO 22
#define CURRENT_SOURCE_DATA_LATCH_PORT	22

// This constant determines how many packages of SPI data is of type 'writing'
// This constant is related to FREQUENCY_DIVIDER_SPI
#define REPEATING_WRITE_COMMAND			20

// Number of samples that are taken into account in statistical calculations
#define RAW_VALUES_HISTORY_SAMPLES		32

// The factor determines the calculation of the current value based on the data
// received by the SPI
#if SCALE_MEASURED_CURRENT_IN_HALF
#define CURRENT_COEFFICIENT_RAW_TO_AMPERS	(200.0/8192.0)
#else
#define CURRENT_COEFFICIENT_RAW_TO_AMPERS	(200.0/4096.0)
#endif

// The factor determines the calculation of the voltage value based on the data
// received by the SPI
#define VOLTAGE_COEFFICIENT_RAW_TO_VOLTS	(100.0/4096.0)

// The two bits OUT_Byte_1:5 and OUT_Byte_1:7 (see the schematics)
#define SPI_SPECIAL_WRITE_BITS			0xA0u


//..............................................................................
// Local variables
//..............................................................................


spi_inst_t* PS_spi = spi_default;
static uint8_t OutgoingDataSpiBuffer[SPI_BUFFER_LENGTH]; // Data from PC to high-current source
static uint8_t IncomingDataSpiBuffer[SPI_BUFFER_LENGTH]; // Data from power source to PC
static uint8_t RepeatWriting;

// The last samples received via SPI (cyclic buffer)
static uint16_t RawCurrentHistory[RAW_VALUES_HISTORY_SAMPLES];
static uint16_t RawVoltageHistory[RAW_VALUES_HISTORY_SAMPLES];
static uint8_t RawHistoryIndex;

static double StatisticsMean, StatisticsMedian, StatisticsMixedMeanMedian, StatisticsPeakToPeak, StatisticsStandardDeviation;
static uint16_t RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES];

#if MODBUS_DEBUG_MODE
static uint8_t HeaderCounter;
static uint16_t CurrentRawValue, VoltageRawValue;
static double CurrentValue, VoltageValue;
#endif

#if SPI_SIMULATION
char SimulationFrameError = 0;

static uint32_t SimulationCounter;
static uint32_t SimulationCountStart = 200;
static uint32_t SimulationCountStop  = 900;
static uint8_t SimulationCommandActive = 0;
static uint8_t SimulationRandomVariation = 0;
static uint8_t SimulationIirTiming = 0;

static bool SimulationPowerOn, SimulationRemote, SimulationSumError;
static uint16_t SimulationCurrentInt, SimulationInt16;
static double SimulationRandomFloat, SimulationIirCurrentFloat, SimulationIirTimingConstant1, SimulationIirTimingConstant2, SimulationCurrentFloat, SimulationVoltageFloat;
#endif


//..............................................................................
// Local functions prototypes
//..............................................................................


static void communicateSPI(void);
static int calculateStatisticsSortFunction(const void *Element1Ptr, const void *Element2Ptr);
static uint8_t reversedByte(uint8_t Data);


//..............................................................................
// Definitions of interface functions
//..............................................................................

// Initialization of hardware peripherals and initialization of variables that
// are used to communicate with the power source via SPI.
void initializeSPI(void){
	spi_init(PS_spi, SPI_CLOCK_FREQUENCY);

	gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
	gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

	gpio_init(CURRENT_SOURCE_DATA_LATCH_PORT);
	gpio_set_dir(CURRENT_SOURCE_DATA_LATCH_PORT, GPIO_OUT);

	memset(&RawCurrentHistory[0],0,sizeof(RawCurrentHistory));
	memset(&RawVoltageHistory[0],0,sizeof(RawVoltageHistory));
	memset(&RawHistorySorted[0],0,sizeof(RawHistorySorted));
	RawHistoryIndex=0;
	StatisticsMean=0.0;
	StatisticsMedian=0.0;
	StatisticsMixedMeanMedian=0.0;
	StatisticsPeakToPeak=0.0;
	StatisticsStandardDeviation=0.0;

	RepeatWriting = 0;

#if SPI_SIMULATION
	SimulationPowerOn = false;
	SimulationRemote = true;
	SimulationSumError = false;
	SimulationCurrentInt = 0;
#endif
}

// This function is to be cyclically called to communicate via SPI with the power source.
// This function stores current and voltage samples but does not calculate their statistical values.
// The function handles special situations described in the documentation (such as appearance of
// errors in the PSU)
bool communicateHighCurrentSource(bool IsWriting){
	static_assert(2 == sizeof(ModbusRegisters[0]), "Error (static_assert)");
	static_assert(1 == sizeof(OutgoingDataSpiBuffer[0]), "Error (static_assert)");
	static_assert(1 == sizeof(IncomingDataSpiBuffer[0]), "Error (static_assert)");
	static_assert(MODBUS_AREA_RW_REGISTERS >= 2, "Error (static_assert)");
	static_assert(MODBUS_AREA_RO_REGISTERS >= 3, "Error (static_assert)");
	static_assert(SPI_BUFFER_LENGTH >= 6, "Error (static_assert)");

	uint8_t J;
	bool SpecificWriteRequest;

	SpecificWriteRequest = false;

	for(J=0; J<6; J++){
		OutgoingDataSpiBuffer[J] = 0;
	}
	if(ModbusRegisters[0] & 1){	// Bit "Power On"
		OutgoingDataSpiBuffer[4] = 2;
	}
	if(((ModbusRegisters[MODBUS_REGISTER_STATUS] & (MODBUS_STATUS_LOCAL_REMOTE | MODBUS_STATUS_POWER_ON)) == (MODBUS_STATUS_LOCAL_REMOTE | MODBUS_STATUS_POWER_ON)) &&
			((ModbusRegisters[MODBUS_REGISTER_STATUS] & (MODBUS_STATUS_EXTERNAL_ERROR | MODBUS_STATUS_SUM_ERROR)) == 0))
	{
		OutgoingDataSpiBuffer[3] = reversedByte((uint8_t)(ModbusRegisters[1] >> 8));      // Set value of the output current
		OutgoingDataSpiBuffer[2] = reversedByte((uint8_t)(ModbusRegisters[1] & 0xFFu));
	}

	if(IsWriting){

#if WRITING_ERROR_TEST
		if(readInputPortJP3()){	// The jumper JP3 blocks write signal to power current source
			OutgoingDataSpiBuffer[4] |= SPI_SPECIAL_WRITE_BITS;
			RepeatWriting = REPEATING_WRITE_COMMAND;
		}
#else
		OutgoingDataSpiBuffer[4] |= SPI_SPECIAL_WRITE_BITS;
		RepeatWriting = REPEATING_WRITE_COMMAND;
#endif

	}
	else{
		if(RepeatWriting > 0){
			OutgoingDataSpiBuffer[4] |= SPI_SPECIAL_WRITE_BITS;
			RepeatWriting--;
		}
	}

#if MODBUS_DEBUG_MODE
	if(IsJumperJP2 && IsWriting){
		if(0 == HeaderCounter){
			printf(" spi output      | spi input       |  I  |  V  |pwr|r|e| mean| medn|mixed| p-p |sDev|simulat.|\r\n");
			HeaderCounter = 30;
		}
		else{
			HeaderCounter--;
		}
		if(IsChangeModbusWrite){
			for(J=0;J<SPI_BUFFER_LENGTH-1;J++){
				printf("%02X ", OutgoingDataSpiBuffer[J]);
			}
			printf("%02X", OutgoingDataSpiBuffer[J]);
		}
	}
#endif // MODBUS_DEBUG_MODE


	communicateSPI();

#if SPI_SIMULATION

	if(IsWriting){
		if((OutgoingDataSpiBuffer[4] & 2) != 0){
			SimulationPowerOn = true;
		}
		else{
			SimulationPowerOn = false;
		}
		// calculate the set-point value
		SimulationCurrentInt =  ((uint16_t)reversedByte(OutgoingDataSpiBuffer[3])) << 8;
		SimulationCurrentInt +=  (uint16_t)reversedByte(OutgoingDataSpiBuffer[2]);
	}
	SimulationRandomFloat = drand48() - 0.5;
	for(J=0;J<9;J++){
		SimulationRandomFloat += drand48() - 0.5;
	}
	SimulationRandomFloat *= (double)SimulationCurrentInt/150.0; // The value of the constant was picked experimentally

	// calculate the simulation value corresponding to the set-point value
#if SCALE_MEASURED_CURRENT_IN_HALF
	SimulationCurrentFloat = (double)SimulationCurrentInt / 8.0;
#else
	SimulationCurrentFloat = (double)SimulationCurrentInt / 16.0; // 65536/4096 = 16
#endif

	// simulation of inertia of the power supply unit
	if (SimulationIirTiming != 0){
		SimulationCurrentFloat    *= SimulationIirTimingConstant1;
		SimulationIirCurrentFloat *= SimulationIirTimingConstant2;
		SimulationIirCurrentFloat += SimulationCurrentFloat;
		SimulationCurrentFloat = SimulationIirCurrentFloat;
	}

	if (SimulationRandomVariation != 0){
		SimulationCurrentFloat += SimulationRandomFloat;
	}
	if(0.0 > SimulationCurrentFloat){
		SimulationCurrentFloat = 0.0;
	}
	SimulationVoltageFloat = SimulationCurrentFloat * 0.12;	// The value of the constant was picked experimentally

#if SPI_SIMULATION_JP4
	if(!readInputPortJP4()){	// If there is a JP4 jumper, the date in the IncomingDataSpiBuffer is replaced with simulation values
#else
	if(true){
#endif
		IncomingDataSpiBuffer[0] = 0x43u; // id (the value of the constant was picked experimentally)
		SimulationInt16 = (uint16_t)(SimulationCurrentFloat+0.5);
		IncomingDataSpiBuffer[1] = (uint8_t)(SimulationInt16 & 0xFFu);
		IncomingDataSpiBuffer[2] = (uint8_t)(SimulationInt16 >> 8);
		SimulationInt16 = (uint16_t)(SimulationVoltageFloat+0.5);
		IncomingDataSpiBuffer[3] = (uint8_t)(SimulationInt16 & 0xFFu);
		IncomingDataSpiBuffer[4] = (uint8_t)(SimulationInt16 >> 8);
		IncomingDataSpiBuffer[5] = 0x74 + (SimulationPowerOn? 8 : 0); //  (the values of the constants were picked experimentally)

#if 1 // temporary failure simulation
		if (0 == SimulationCommandActive){	// the simulation is not running

			if (IsWriting){
				// only new command can start the simulation if parameters are good
				if (ModbusRegisters[1] == 0x47B2u ){
					// start simulation of temporary state change to 'LOCAL'
					SimulationCommandActive = 1;
					printf("simulation of 'LOCAL'\n");
				}
				else if (ModbusRegisters[1] == 0x47B3u ){
					// start simulation of temporary state change to 'POWER OFF'
					SimulationCommandActive = 2;
					printf("simulation of 'POWER OFF'\n");
				}
				else if (ModbusRegisters[1] == 0x47B4u ){
					// start simulation of temporary state change to 'Sum.Err.'
					SimulationCommandActive = 3;
					printf("simulation of 'Sum.Err.'\n");
				}
				else if (ModbusRegisters[1] == 0x47B5u ){
					// start simulation of temporary state change to 'Ext.Err.'
					SimulationCommandActive = 4;
					printf("simulation of 'Ext.Err.'\n");
				}
				else if (ModbusRegisters[1] == 0x47B6u ){
					// start simulation of temporary change of physical ID
					SimulationCommandActive = 5;
					printf("simulation of physical ID\n");
				}
				else if (ModbusRegisters[1] == 0x47B7u ){
					SimulationCommandActive = 6;
					printf("simulation of LOCAL + Sum.Err.\n");
				}
				else if (ModbusRegisters[1] == 0x47B8u ){
					SimulationCommandActive = 7;
					printf("simulation of LOCAL + Ext.Err.\n");
				}
				else if (ModbusRegisters[1] == 0x47B9u ){
					SimulationCommandActive = 8;
					printf("simulation of Sum.Err.+Ext.Err.\n");
				}
				else if (ModbusRegisters[1] == 0x47BAu ){
					SimulationCommandActive = 9;
					printf("simulation of all\n");
				}
				else if (ModbusRegisters[1] == 0x47BBu ){
					SimulationCommandActive = 10;
					printf("simulation of frame err\n");
				}
				else if (ModbusRegisters[1] == 0x47BCu ){
					SimulationRandomVariation = 1;
					printf("simulation of random variation\n");
				}
				else if (ModbusRegisters[1] == 0x47BDu ){
					SimulationRandomVariation = 0;
					printf("stop random variation\n");
				}
				else if (ModbusRegisters[1] == 0x47BEu ){
					SimulationIirTiming = 1;
					SimulationIirTimingConstant1 = 0.01;
					SimulationIirTimingConstant2 = 0.99;
					SimulationIirCurrentFloat = SimulationCurrentFloat;
					printf("moderate slowdown in output current changes\n");
				}
				else if (ModbusRegisters[1] == 0x47BFu ){
					SimulationIirTiming = 1;
					SimulationIirTimingConstant1 = 0.001;
					SimulationIirTimingConstant2 = 0.999;
					SimulationIirCurrentFloat = SimulationCurrentFloat;
					printf("large slowdown in output current changes\n");
				}
				else if (ModbusRegisters[1] == 0x47C0u ){
					SimulationIirTiming = 0;
					SimulationIirTimingConstant1 = 1.0;
					SimulationIirTimingConstant2 = 0.0;
					printf("disabled slowdown in output current changes\n");
				}
				else{
					// unknown command: do nothing
				}
			}
		}
		else{
			// the simulation is running
			if ((SimulationCounter < SimulationCountStop)){
				if (SimulationCounter >= SimulationCountStart){
					if (1 == SimulationCommandActive){
						// temporary state change to 'LOCAL'
						IncomingDataSpiBuffer[5] &= (0xFFu - 0x04u);
					}
					else if (2 == SimulationCommandActive){
						// temporary state change to 'POWER OFF'
						IncomingDataSpiBuffer[5] &= (0xFFu - 0x08u);
					}
					else if (3 == SimulationCommandActive){
						// temporary state change to 'Sum.Err.'
						IncomingDataSpiBuffer[5] |= 0x01u;
					}
					else if (4 == SimulationCommandActive){
						// temporary state change to 'Ext.Err.'
						IncomingDataSpiBuffer[5] |= 0x02u;
					}
					else if (5 == SimulationCommandActive){
						// temporary change of physical ID
						IncomingDataSpiBuffer[0] = 0x0Du;
					}
					else if (6 == SimulationCommandActive){
						// LOCAL + Sum.Err.
						IncomingDataSpiBuffer[5] &= (0xFFu - 0x04u);
						IncomingDataSpiBuffer[5] |= 0x01u;
					}
					else if (7 == SimulationCommandActive){
						// LOCAL + Ext.Err.
						IncomingDataSpiBuffer[5] &= (0xFFu - 0x04u);
						IncomingDataSpiBuffer[5] |= 0x02u;
					}
					else if (8 == SimulationCommandActive){
						// Sum.Err. + Ext.Err.
						IncomingDataSpiBuffer[5] |= 0x03u;
					}
					else if (9 == SimulationCommandActive){
						// all
						IncomingDataSpiBuffer[5] &= (0xFFu - 0x04u);
						IncomingDataSpiBuffer[5] |= 0x03u;
					}
					else if (10 == SimulationCommandActive){
						// frame err
						SimulationFrameError = 1;
					}
					else{
						// internal error: do nothing
					}
					if (SimulationCounter == SimulationCountStart){
						printf("simulation start\n");
					}
				}
				SimulationCounter++;
			}
			else{
				// reset the simulation state machine
				SimulationCommandActive = 0;
				SimulationCounter = 0;
				SimulationFrameError = 0;
				printf("simulation completed\n");
			}
		}
#endif // #if 1 // temporary failure simulation

	}
#endif // #if SPI_SIMULATION

	if(!IsWriting){
		if(RawHistoryIndex >= RAW_VALUES_HISTORY_SAMPLES){
			RawHistoryIndex=0;
		}
		RawCurrentHistory[RawHistoryIndex]=(uint16_t)IncomingDataSpiBuffer[1]+256*(uint16_t)IncomingDataSpiBuffer[2];
		RawVoltageHistory[RawHistoryIndex]=(uint16_t)IncomingDataSpiBuffer[3]+256*(uint16_t)IncomingDataSpiBuffer[4];
		RawHistoryIndex++;
	}

	ModbusRegisters[MODBUS_REGISTER_ID]   = (uint16_t)IncomingDataSpiBuffer[0]; // Id register

	ModbusRegisters[MODBUS_REGISTER_STATUS] = 0;			// Status register
	if((IncomingDataSpiBuffer[5] & 0x08u) != 0){
		ModbusRegisters[MODBUS_REGISTER_STATUS] =  MODBUS_STATUS_POWER_ON;
	}
	else{
#if INTERVENTION_SHUTDOWN_ON_POWER_OFF
		// In the POWER OFF state the output current setting value should be zeroed.
		if (0 != ModbusRegisters[MODBUS_REGISTER_SET_CURRENT]){
			ModbusRegisters[MODBUS_REGISTER_SET_CURRENT] = 0;
			SpecificWriteRequest = true; // This SPI write request is caused by a specific change in the state of the power supply unit
		}
#endif
	}
	if((IncomingDataSpiBuffer[5] & 0x04u) != 0){
		ModbusRegisters[MODBUS_REGISTER_STATUS] |= MODBUS_STATUS_LOCAL_REMOTE;
	}
	else{
#if INTERVENTION_SHUTDOWN_ON_LOCAL_MODE
		// If the user set LOCAL mode then settings of output current value should be zeroed.
		if ((0 != ModbusRegisters[MODBUS_REGISTER_SET_POWER]) || (0 != ModbusRegisters[MODBUS_REGISTER_SET_CURRENT])){
			ModbusRegisters[MODBUS_REGISTER_SET_POWER] = 0;
			ModbusRegisters[MODBUS_REGISTER_SET_CURRENT] = 0;
			SpecificWriteRequest = true; // This SPI write request is caused by a specific change in the state of the power supply unit
		}
#endif
	}
	if((IncomingDataSpiBuffer[5] & 0x02u) != 0){
		ModbusRegisters[MODBUS_REGISTER_STATUS] |= MODBUS_STATUS_EXTERNAL_ERROR;
#if INTERVENTION_SHUTDOWN_ON_SUM_ERROR
		// If any errors occurred then settings of Power On value and output current value should be zeroed.
		if ((0 != ModbusRegisters[MODBUS_REGISTER_SET_POWER]) || (0 != ModbusRegisters[MODBUS_REGISTER_SET_CURRENT])){
			ModbusRegisters[MODBUS_REGISTER_SET_POWER] = 0;
			ModbusRegisters[MODBUS_REGISTER_SET_CURRENT] = 0;
			SpecificWriteRequest = true; // This SPI write request is caused by a specific change in the state of the power supply unit
		}
#endif
	}
	if((IncomingDataSpiBuffer[5] & 0x01u) != 0){
		ModbusRegisters[MODBUS_REGISTER_STATUS] |= MODBUS_STATUS_SUM_ERROR;
#if INTERVENTION_SHUTDOWN_ON_EXT_ERROR
		// If any errors occurred then settings of Power On value and output current value should be zeroed.
		if ((0 != ModbusRegisters[MODBUS_REGISTER_SET_POWER]) || (0 != ModbusRegisters[MODBUS_REGISTER_SET_CURRENT])){
			ModbusRegisters[MODBUS_REGISTER_SET_POWER] = 0;
			ModbusRegisters[MODBUS_REGISTER_SET_CURRENT] = 0;
			SpecificWriteRequest = true; // This SPI write request is caused by a specific change in the state of the power supply unit
		}
#endif
	}

#if MODBUS_DEBUG_MODE
	if(IsJumperJP2 && IsWriting){
		if (!IsChangeModbusWrite){
			printf("                 ");
		}

		printf("|");
		for(J=0;J<SPI_BUFFER_LENGTH-1;J++){
			printf("%02X ", IncomingDataSpiBuffer[J]);
		}
		printf("%02X", IncomingDataSpiBuffer[J]);

		// Interpretation of data based on experiments
		CurrentRawValue=(uint16_t)IncomingDataSpiBuffer[1]+256*(uint16_t)IncomingDataSpiBuffer[2];
		VoltageRawValue=(uint16_t)IncomingDataSpiBuffer[3]+256*(uint16_t)IncomingDataSpiBuffer[4];

		CurrentValue=((double)CurrentRawValue)*CURRENT_COEFFICIENT_RAW_TO_AMPERS;
		VoltageValue=((double)VoltageRawValue)*(100.0/4096.0);

		printf("|%5.1f|%5.1f|", CurrentValue, VoltageValue );

		if((IncomingDataSpiBuffer[5] & 0x08u) != 0){
			printf("ON |");
		}
		else{
			printf("OFF|");
		}
		if((IncomingDataSpiBuffer[5] & 0x04u) != 0){
			printf("R|");
		}
		else{
			printf("L|");
		}
		if((IncomingDataSpiBuffer[5] & 0x01u) != 0){
			printf("E|");
		}
		else{
			printf(" |");
		}

		calculateStatistics();
		printf("%5.1f|%5.1f|%5.1f|%5.1f|%4.2f|", StatisticsMean, StatisticsMedian, StatisticsMixedMeanMedian, StatisticsPeakToPeak, StatisticsStandardDeviation );

#if SPI_SIMULATION
		if(SimulationPowerOn){
			printf("ON |");
		}
		else{
			printf("OFF|");
		}
		printf("%4d|",SimulationCurrentInt);
#endif

#if DISPLAY_ALL_SAMPLES
		for(J=0;J<RAW_VALUES_HISTORY_SAMPLES;J++){
			if(J%16 == 0){
				printf("\r\n");
			}
			printf("%6u ",RawCurrentHistory[J]);
		}
#endif // DISPLAY_ALL_SAMPLES

		printf("\r\n");
	}

#endif // MODBUS_DEBUG_MODE
	return SpecificWriteRequest;
}

// The function calculates statistical values of current and voltage based on samples
// stored in memory.
void calculateStatistics(void){
	uint8_t L;
	double RawMean, M;

	// Estimates and other statistics of output voltage of Svedberg power supply

	memcpy(&RawHistorySorted[0],&RawVoltageHistory[0],sizeof(RawHistorySorted));
	qsort((void*)&RawHistorySorted[0],RAW_VALUES_HISTORY_SAMPLES,sizeof(RawHistorySorted[0]),calculateStatisticsSortFunction);

	RawMean=0.0;
	for(L=0;L<RAW_VALUES_HISTORY_SAMPLES;L++){
		RawMean+=RawHistorySorted[L];
	}
	RawMean *= 1.0/(double)RAW_VALUES_HISTORY_SAMPLES;
	StatisticsMean = RawMean * VOLTAGE_COEFFICIENT_RAW_TO_VOLTS;

	StatisticsMedian = (double)(RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES/2-1]+RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES/2]);
	StatisticsMedian *= VOLTAGE_COEFFICIENT_RAW_TO_VOLTS / 2.0;

	StatisticsMixedMeanMedian=0.0;
	for(L=RAW_VALUES_HISTORY_SAMPLES/4;L<RAW_VALUES_HISTORY_SAMPLES-RAW_VALUES_HISTORY_SAMPLES/4;L++){
		StatisticsMixedMeanMedian+=RawHistorySorted[L];
	}
	StatisticsMixedMeanMedian *= VOLTAGE_COEFFICIENT_RAW_TO_VOLTS / (double)(RAW_VALUES_HISTORY_SAMPLES / 2);

	StatisticsPeakToPeak = (double)RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES-1] - (double)RawHistorySorted[0];
	StatisticsPeakToPeak *= VOLTAGE_COEFFICIENT_RAW_TO_VOLTS;

	StatisticsStandardDeviation=0.0;
	for(L=0;L<RAW_VALUES_HISTORY_SAMPLES;L++){
		M = (double)RawHistorySorted[L] - RawMean;
		StatisticsStandardDeviation += M*M;
	}
	StatisticsStandardDeviation *= 1.0/(double)(RAW_VALUES_HISTORY_SAMPLES-1);
	StatisticsStandardDeviation = sqrt(StatisticsStandardDeviation);
	StatisticsStandardDeviation *= VOLTAGE_COEFFICIENT_RAW_TO_VOLTS;

	ModbusRegisters[MODBUS_REGISTER_V_MEAN]		= (uint16_t)lrint(StatisticsMean*100.0);
	ModbusRegisters[MODBUS_REGISTER_V_MEDIAN]	= (uint16_t)lrint(StatisticsMedian*100.0);
	ModbusRegisters[MODBUS_REGISTER_V_MIXED]	= (uint16_t)lrint(StatisticsMixedMeanMedian*100.0);
	ModbusRegisters[MODBUS_REGISTER_V_PEAK_PEAK]= (uint16_t)lrint(StatisticsPeakToPeak*100.0);
	ModbusRegisters[MODBUS_REGISTER_V_STD_DEV]	= (uint16_t)lrint(StatisticsStandardDeviation*100.0);

	// Estimates and other statistics of output current of Svedberg power supply

	memcpy(&RawHistorySorted[0],&RawCurrentHistory[0],sizeof(RawHistorySorted));
	qsort((void*)&RawHistorySorted[0],RAW_VALUES_HISTORY_SAMPLES,sizeof(RawHistorySorted[0]),calculateStatisticsSortFunction);

	RawMean=0.0;
	for(L=0;L<RAW_VALUES_HISTORY_SAMPLES;L++){
		RawMean+=RawHistorySorted[L];
	}
	RawMean *= 1.0/(double)RAW_VALUES_HISTORY_SAMPLES;
	StatisticsMean = RawMean * CURRENT_COEFFICIENT_RAW_TO_AMPERS;

	StatisticsMedian = (double)(RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES/2-1]+RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES/2]);
	StatisticsMedian *= CURRENT_COEFFICIENT_RAW_TO_AMPERS / 2.0;

	StatisticsMixedMeanMedian=0.0;
	for(L=RAW_VALUES_HISTORY_SAMPLES/4;L<RAW_VALUES_HISTORY_SAMPLES-RAW_VALUES_HISTORY_SAMPLES/4;L++){
		StatisticsMixedMeanMedian+=RawHistorySorted[L];
	}
	StatisticsMixedMeanMedian *= CURRENT_COEFFICIENT_RAW_TO_AMPERS / (double)(RAW_VALUES_HISTORY_SAMPLES / 2);

	StatisticsPeakToPeak = (double)RawHistorySorted[RAW_VALUES_HISTORY_SAMPLES-1] - (double)RawHistorySorted[0];
	StatisticsPeakToPeak *= CURRENT_COEFFICIENT_RAW_TO_AMPERS;

	StatisticsStandardDeviation=0.0;
	for(L=0;L<RAW_VALUES_HISTORY_SAMPLES;L++){
		M = (double)RawHistorySorted[L] - RawMean;
		StatisticsStandardDeviation += M*M;
	}
	StatisticsStandardDeviation *= 1.0/(double)(RAW_VALUES_HISTORY_SAMPLES-1);
	StatisticsStandardDeviation = sqrt(StatisticsStandardDeviation);
	StatisticsStandardDeviation *= CURRENT_COEFFICIENT_RAW_TO_AMPERS;

	ModbusRegisters[MODBUS_REGISTER_I_MEAN]		= (uint16_t)lrint(StatisticsMean*100.0);
	ModbusRegisters[MODBUS_REGISTER_I_MEDIAN]	= (uint16_t)lrint(StatisticsMedian*100.0);
	ModbusRegisters[MODBUS_REGISTER_I_MIXED]	= (uint16_t)lrint(StatisticsMixedMeanMedian*100.0);
	ModbusRegisters[MODBUS_REGISTER_I_PEAK_PEAK]= (uint16_t)lrint(StatisticsPeakToPeak*100.0);
	ModbusRegisters[MODBUS_REGISTER_I_STD_DEV]	= (uint16_t)lrint(StatisticsStandardDeviation*100.0);
}


//..............................................................................
// Local function definitions
//..............................................................................


static void communicateSPI(void){
    uint8_t K;
    gpio_put(CURRENT_SOURCE_DATA_LATCH_PORT,true);

    for(K=0;SPI_BUFFER_LENGTH>K;K++ ){
        spi_write_read_blocking(PS_spi, &OutgoingDataSpiBuffer[K], &IncomingDataSpiBuffer[K], 1);
    }

    gpio_put(CURRENT_SOURCE_DATA_LATCH_PORT,false);
}

static int calculateStatisticsSortFunction(const void *Element1Ptr, const void *Element2Ptr){
	if(((uint16_t*)Element1Ptr)[0]<((uint16_t*)Element2Ptr)[0]){
		return(-1);
	}
	return(1);
}

static uint8_t reversedByte(uint8_t Data){
	uint8_t Result;
	Result = 0;
	if(Data & 0x01u){
		Result |= 0x80u;
	}
	if(Data & 0x02u){
		Result |= 0x40u;
	}
	if(Data & 0x04u){
		Result |= 0x20u;
	}
	if(Data & 0x08u){
		Result |= 0x10u;
	}
	if(Data & 0x10u){
		Result |= 0x08u;
	}
	if(Data & 0x20u){
		Result |= 0x04u;
	}
	if(Data & 0x40u){
		Result |= 0x02u;
	}
	if(Data & 0x80u){
		Result |= 0x01u;
	}
	return(Result);
}

