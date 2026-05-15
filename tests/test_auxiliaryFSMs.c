/// @file test_auxiliaryFSMs.c
/// Unit tests for AuxiliaryFSMs module.

#include "auxiliaryFSMs.h"
#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define ASSERT(condition, message) \
    do { \
        test_count++; \
        if (!(condition)) { \
            fprintf(stderr, "FAIL [%d]: %s\n", test_count, message); \
            test_failed++; \
        } else { \
            printf("PASS [%d]: %s\n", test_count, message); \
            test_passed++; \
        } \
    } while (0)

#define ASSERT_EQ(actual, expected, message) \
    ASSERT((actual) == (expected), message)

static void init_inputs(AuxiliaryFSMsInputs *inputs)
{
    memset(inputs, 0, sizeof(*inputs));
    inputs->installed_cups = 3u;

    for (uint16_t i = 0; i < AUXILIARY_FSMS_MAX_CUPS; i++) {
        inputs->cup_type[i] = AUXILIARY_FSM_CUP_TYPE_PNEUMATIC;
        inputs->time_limit_inserting_ms[i] = 10u;
        inputs->time_limit_withdrawing_ms[i] = 10u;
    }
}

static void init_state(AuxiliaryFSMsState *state)
{
    memset(state, 0, sizeof(*state));
}

static void test_pneumatic_steady_inserted(void)
{
    AuxiliaryFSMsInputs inputs;
    AuxiliaryFSMsState state;
    AuxiliaryFSMsOutputs outputs;

    init_inputs(&inputs);
    init_state(&state);

    inputs.cup_requested_state[0] = true;
    inputs.cup_switch[0] = true;

    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);

    ASSERT_EQ(outputs.actuator_control[0], true, "Pneumatic: actuator follows request");
    ASSERT_EQ(outputs.cup_inserted[0], true, "Pneumatic: switch=true means inserted");
    ASSERT_EQ(outputs.cup_steady[0], true, "Pneumatic: inserted and requested means steady");
    ASSERT_EQ(outputs.cup_error[0], 0u, "Pneumatic: no error in steady state");
}

static void test_pneumatic_timeout_insert(void)
{
    AuxiliaryFSMsInputs inputs;
    AuxiliaryFSMsState state;
    AuxiliaryFSMsOutputs outputs;

    init_inputs(&inputs);
    init_state(&state);

    inputs.cup_requested_state[0] = true;
    inputs.cup_switch[0] = false;
    inputs.time_limit_inserting_ms[0] = 5u;

    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);
    ASSERT_EQ(outputs.cup_error[0], 0u, "Timeout: no error before limit");

    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);
    ASSERT_EQ(outputs.cup_error[0], 0u, "Timeout: still no error at 4 ms");

    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);
    ASSERT((outputs.cup_error[0] & AUXILIARY_FSM_ERROR_TIMEOUT_INSERT) != 0u,
           "Timeout: insert timeout is raised after limit");
    ASSERT((outputs.cup_error_storage[0] & AUXILIARY_FSM_ERROR_TIMEOUT_INSERT) != 0u,
           "Timeout: storage accumulates timeout");
}

static void test_motor_states_and_invalid_switches(void)
{
    AuxiliaryFSMsInputs inputs;
    AuxiliaryFSMsState state;
    AuxiliaryFSMsOutputs outputs;

    init_inputs(&inputs);
    init_state(&state);

    inputs.cup_type[2] = AUXILIARY_FSM_CUP_TYPE_MOTOR;
    inputs.cup_requested_state[2] = true;

    inputs.cup_switch1[2] = false;
    inputs.cup_switch2[2] = true;
    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);

    ASSERT_EQ(outputs.cup_inserted[2], true, "Motor: switch pattern 01 means inserted");
    ASSERT_EQ(outputs.cup_steady[2], true, "Motor: requested insert + inserted means steady");
    ASSERT_EQ(outputs.cup_error[2], 0u, "Motor: valid steady state has no error");

    inputs.cup_switch1[2] = true;
    inputs.cup_switch2[2] = true;
    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);

    ASSERT((outputs.cup_error[2] & AUXILIARY_FSM_ERROR_INVALID_SWITCHES) != 0u,
           "Motor: switch pattern 11 is invalid");
}

static void test_last_error_on_transition_to_no_error(void)
{
    AuxiliaryFSMsInputs inputs;
    AuxiliaryFSMsState state;
    AuxiliaryFSMsOutputs outputs;

    init_inputs(&inputs);
    init_state(&state);

    inputs.cup_requested_state[0] = true;
    inputs.cup_switch[0] = false;
    inputs.time_limit_inserting_ms[0] = 1u;

    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);
    ASSERT((outputs.cup_error[0] & AUXILIARY_FSM_ERROR_TIMEOUT_INSERT) != 0u,
           "LastError: timeout active");

    inputs.cup_switch[0] = true;
    auxiliaryFSMsTick(&inputs, &state, &outputs, 2u);

    ASSERT_EQ(outputs.cup_error[0], 0u, "LastError: error cleared in steady state");
    ASSERT((outputs.cup_last_error[0] & AUXILIARY_FSM_ERROR_TIMEOUT_INSERT) != 0u,
           "LastError: previous error is reported");
}

int main(void)
{
    printf("========================================\n");
    printf("AuxiliaryFSMs Unit Tests\n");
    printf("========================================\n");

    test_pneumatic_steady_inserted();
    test_pneumatic_timeout_insert();
    test_motor_states_and_invalid_switches();
    test_last_error_on_transition_to_no_error();

    printf("\n========================================\n");
    printf("Test Results: %d/%d passed\n", test_passed, test_count);
    printf("Failed: %d\n", test_failed);
    printf("========================================\n");

    return (test_failed == 0) ? 0 : 1;
}
