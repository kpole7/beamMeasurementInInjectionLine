/// @file logicInputs.c
// This source file was written by K.O. (2026)

#include "logicInputs.h"
#include "sharedData.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define GPIO_FOR_LIMIT_SWITCH_1 2
#define GPIO_FOR_LIMIT_SWITCH_2 3
#define GPIO_FOR_LIMIT_SWITCH_3A 21
#define GPIO_FOR_LIMIT_SWITCH_3B 20
#define GPIO_FOR_EXTERNAL_INHIBITION 13

#define LIMIT_SWITCH_1_INDEX 0
#define LIMIT_SWITCH_2_INDEX 1
#define LIMIT_SWITCH_3A_INDEX 2
#define LIMIT_SWITCH_3B_INDEX 3
#define EXTERNAL_INHIBITION_INDEX 4

#define DEBOUNCE_TICKS 5

static bool StableState[5];
static bool TemporaryState[5];
static uint16_t Counter[5];

void initializeLogicInputs(void) {
    gpio_init(GPIO_FOR_LIMIT_SWITCH_1);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_1, GPIO_IN);
    StableState[LIMIT_SWITCH_1_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_1);

    gpio_init(GPIO_FOR_LIMIT_SWITCH_2);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_2, GPIO_IN);
    StableState[LIMIT_SWITCH_2_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_2);

    gpio_init(GPIO_FOR_LIMIT_SWITCH_3A);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_3A, GPIO_IN);
    StableState[LIMIT_SWITCH_3A_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_3A);

    gpio_init(GPIO_FOR_LIMIT_SWITCH_3B);
    gpio_set_dir(GPIO_FOR_LIMIT_SWITCH_3B, GPIO_IN);
    StableState[LIMIT_SWITCH_3B_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_3B);

    gpio_init(GPIO_FOR_EXTERNAL_INHIBITION);
    gpio_set_dir(GPIO_FOR_EXTERNAL_INHIBITION, GPIO_IN);
    StableState[EXTERNAL_INHIBITION_INDEX] = gpio_get(GPIO_FOR_EXTERNAL_INHIBITION);

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
    CurrentState[LIMIT_SWITCH_1_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_1);
    CurrentState[LIMIT_SWITCH_2_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_2);
    CurrentState[LIMIT_SWITCH_3A_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_3A);
    CurrentState[LIMIT_SWITCH_3B_INDEX] = gpio_get(GPIO_FOR_LIMIT_SWITCH_3B);
    CurrentState[EXTERNAL_INHIBITION_INDEX] = gpio_get(GPIO_FOR_EXTERNAL_INHIBITION);

    for (int J = 0; J < 5; J++) {
        if (CurrentState[J] != StableState[J]) {
            if (CurrentState[J] == TemporaryState[J]) {
                Counter[J]++;
                if (Counter[J] >= DEBOUNCE_TICKS) {
                    StableState[J] = CurrentState[J];
                    Counter[J] = 0;
                    // Update Modbus registers or coils based on the new stable state
                    switch (J) {
                        case LIMIT_SWITCH_1_INDEX:
                            ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP1_SWITCH)] = StableState[J];
                            break;
                        case LIMIT_SWITCH_2_INDEX:
                            ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP2_SWITCH)] = StableState[J];
                            break;
                        case LIMIT_SWITCH_3A_INDEX:
                            ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH1)] = StableState[J];
                            break;
                        case LIMIT_SWITCH_3B_INDEX:
                            ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_CUP3_SWITCH2)] = StableState[J];
                            break;
                        case EXTERNAL_INHIBITION_INDEX:
                            ModbusCoils[coilIndexFromAddress(MODBUS_ADDR_EXTERNAL_INHIBITION)] = StableState[J];
                            break;
                    }
                }
            } else {
                Counter[J] = 0;
            }
        }
        TemporaryState[J] = CurrentState[J];
    }

    // just for testing purposes
    static bool OldStableState[5] = {0};
    char DebugNames[5][5] = {"sw1", "sw2", "sw3A", "sw3B", "INH"};
    if ((ModbusHoldingRegisters[holdingIndexFromAddress(MODBUS_ADDR_DEBUG_PRINTOUTS)] & 4u) != 0u){
        for (int J = 0; J < 5; J++) {
            if (StableState[J] != OldStableState[J]) {
                printf("Input %s changed to %d\r\n", DebugNames[J], (int)StableState[J]);
                OldStableState[J] = StableState[J];
            }
        }
    }

}
