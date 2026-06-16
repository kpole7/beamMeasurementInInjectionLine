/// @file logicInputs.c
// This source file was written by K.O. (2026)

#include "logicInputs.h"
#include "sharedData.h"
#include "masterConfig.h"
#include "debuggingTools.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define GPIO_FOR_LIMIT_SWITCH_1 2
#define GPIO_FOR_LIMIT_SWITCH_2 3
#define GPIO_FOR_LIMIT_SWITCH_3A 21
#define GPIO_FOR_LIMIT_SWITCH_3B 20
#define GPIO_FOR_EXTERNAL_INHIBITION 13

#define DEBOUNCE_TICKS 5

static bool StableState[5];
static bool TemporaryState[5];
static uint16_t Counter[5];

void initializeLogicInputs(void) {
    gpio_init(GPIO_FOR_LIMIT_SWITCH_1);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_1, GPIO_IN);
    // the Modbus coil shows '1' when the uC pin is at a low voltage level, and '0' when the uC pin is at a high voltage level
    StableState[LIMIT_SWITCH_1_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_1);

    gpio_init(GPIO_FOR_LIMIT_SWITCH_2);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_2, GPIO_IN);
    // the Modbus coil shows '1' when the uC pin is at a low voltage level, and '0' when the uC pin is at a high voltage level
    StableState[LIMIT_SWITCH_2_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_2);

    gpio_init(GPIO_FOR_LIMIT_SWITCH_3A);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_3A, GPIO_IN);
    // the Modbus coil shows '1' when the uC pin is at a low voltage level, and '0' when the uC pin is at a high voltage level
    StableState[LIMIT_SWITCH_3A_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_3A);

    gpio_init(GPIO_FOR_LIMIT_SWITCH_3B);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_3B, GPIO_IN);
    // the Modbus coil shows '1' when the uC pin is at a low voltage level, and '0' when the uC pin is at a high voltage level
    StableState[LIMIT_SWITCH_3B_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_3B);

    gpio_init(GPIO_FOR_EXTERNAL_INHIBITION);
    gpio_set_dir(GPIO_FOR_EXTERNAL_INHIBITION, GPIO_IN);
    StableState[EXTERNAL_INHIBITION_INDEX] = gpio_get(GPIO_FOR_EXTERNAL_INHIBITION);

#if DEBUG_SIMULATION_MODE != 0
    for (int K = 0; K < 4; K++) {
        StableState[K] = simulateInput(K);
    }
    // In the simulation mode, the user can change the state of the external inhibition input directly by Modbus
    StableState[EXTERNAL_INHIBITION_INDEX] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION2)];
#endif

    for (int J = 0; J < 5; J++) {
        TemporaryState[J] = StableState[J];
        Counter[J] = 0;
    }
}

/// @brief This function reads the state of logic inputs
/// This function is called periodically in the main loop to read the state of logic inputs and 
/// update Modbus registers or coils accordingly.
void logicInputsTick(void) {
    bool CurrentState[5];
#if DEBUG_SIMULATION_MODE == 0
    // the Modbus coils show '1' when the uC pins are at a low voltage level, 
    // and '0' when the uC pins are at a high voltage level in the below lines related to the limit switches, 
    // so the values read from the uC pins are inverted
    CurrentState[LIMIT_SWITCH_1_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_1);
    CurrentState[LIMIT_SWITCH_2_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_2);
    CurrentState[LIMIT_SWITCH_3A_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_3A);
    CurrentState[LIMIT_SWITCH_3B_INDEX] = !gpio_get(GPIO_FOR_LIMIT_SWITCH_3B);

    CurrentState[EXTERNAL_INHIBITION_INDEX] = gpio_get(GPIO_FOR_EXTERNAL_INHIBITION);
#else
    for (int K = 0; K < 4; K++) {
        CurrentState[K] = simulateInput(K);
    }
    // In the simulation mode, the user can change the state of the external inhibition input directly by Modbus
    CurrentState[EXTERNAL_INHIBITION_INDEX] = ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION2)];
#endif

    // Debouncing logic: the stable state changes only if the current state is the same for DEBOUNCE_TICKS consecutive ticks
    for (int J = 0; J < 5; J++) {
        if (CurrentState[J] != StableState[J]) {
            if (CurrentState[J] == TemporaryState[J]) {
                Counter[J]++;
                if (Counter[J] >= DEBOUNCE_TICKS) {
                    StableState[J] = CurrentState[J];
                    Counter[J] = 0;
                }
            } else {
                Counter[J] = 0;
            }
        }
        TemporaryState[J] = CurrentState[J];
    }

    // Update Modbus registers or coils based on the new stable state
    ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)] = StableState[LIMIT_SWITCH_1_INDEX];
    ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)] = StableState[LIMIT_SWITCH_2_INDEX];
    ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH_A)] = StableState[LIMIT_SWITCH_3A_INDEX];
    ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH_B)] = StableState[LIMIT_SWITCH_3B_INDEX];
#if DEBUG_SIMULATION_MODE == 0
    // In the simulation mode, no modification is needed
    ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION2)] = StableState[EXTERNAL_INHIBITION_INDEX];
#endif

    // just for testing purposes
	(void)getTimeStampString(); // Update the time stamp string for printouts.
    static bool PrintoutsForTestingPurposes = false;
    if (((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 4u) != 0u) != PrintoutsForTestingPurposes) {
        PrintoutsForTestingPurposes = ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 4u) != 0u);
        if (PrintoutsForTestingPurposes) {
            printf("%s  LIn  printouts enabled\r\n", getTimeStampStringWithoutUpdate());
        }
        else{
            printf("%s  LIn  printouts disabled\r\n", getTimeStampStringWithoutUpdate());
        }
    }
    if (PrintoutsForTestingPurposes){
        static bool OldStableState[5] = {0};
        char DebugNames[5][5] = {"sw1", "sw2", "sw3A", "sw3B", "INH"};
        char *SwitchStateNames[2] = {"released", "pressed"};
        char *InhibitionStateNames[2] = {"not active", "active"};
        for (int J = 0; J < 4; J++) {
            if (StableState[J] != OldStableState[J]) {
                printf("%s  LIn  %s %s\r\n", getTimeStampStringWithoutUpdate(), DebugNames[J], SwitchStateNames[StableState[J]]);
                OldStableState[J] = StableState[J];
            }
        }
        if (StableState[EXTERNAL_INHIBITION_INDEX] != OldStableState[EXTERNAL_INHIBITION_INDEX]) {
            printf("%s  LIn  %s %s\r\n", getTimeStampStringWithoutUpdate(), DebugNames[EXTERNAL_INHIBITION_INDEX], 
                InhibitionStateNames[StableState[EXTERNAL_INHIBITION_INDEX]]);
            OldStableState[EXTERNAL_INHIBITION_INDEX] = StableState[EXTERNAL_INHIBITION_INDEX];
        }
    }
}
