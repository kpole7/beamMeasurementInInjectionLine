/// @file test_highLevelCtrl.c
/// Unit tests for HighLevelCtrl module.
/// Tests Rules 1-4 from documentation/tasks.md.

#include "highLevelCtrl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//==============================================================================
// Simple Test Framework
//==============================================================================

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
    ASSERT((actual) == (expected), message ": expected " #expected " got " #actual)

#define ASSERT_NEQ(actual, expected, message) \
    ASSERT((actual) != (expected), message)

//==============================================================================
// Helper Functions
//==============================================================================

/// Initialize inputs with all zeros
static void init_inputs_zero(HighLevelCtrlInputs *inputs)
{
    memset(inputs, 0, sizeof(HighLevelCtrlInputs));
    inputs->installed_cups = 3;
}

/// Initialize state with safe defaults
static void init_state_default(HighLevelCtrlState *state)
{
    memset(state, 0, sizeof(HighLevelCtrlState));
    state->retained_active_cup = 1;  // Default to cup 1
}

//==============================================================================
// Test Cases
//==============================================================================

void test_rule1_external_inhibition_locks_last_cup()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // Setup: no external inhibition, user wants to withdraw cup 3
    inputs.external_inhibition = false;
    inputs.cup_control[2] = false;  // Cup 3: withdraw
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.cup_requested_state[2], false, "Rule 1: without inhibit, follow user request");
    
    // Setup: external inhibition active, user wants to withdraw cup 3
    inputs.external_inhibition = true;
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.cup_requested_state[2], true, "Rule 1: with inhibit, force cup 3 to insert");
}

void test_rule1_inhibition_only_affects_last_cup()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    inputs.external_inhibition = true;
    inputs.cup_control[0] = false;  // Cup 1: user wants withdraw
    inputs.cup_control[1] = false;  // Cup 2: user wants withdraw
    inputs.cup_control[2] = false;  // Cup 3: user wants withdraw
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    // Cups 1 and 2 should follow user request
    ASSERT_EQ(outputs.cup_requested_state[0], false, "Rule 1: inhibition does not affect cup 1");
    ASSERT_EQ(outputs.cup_requested_state[1], false, "Rule 1: inhibition does not affect cup 2");
    // Cup 3 should be forced to insert
    ASSERT_EQ(outputs.cup_requested_state[2], true, "Rule 1: inhibition forces cup 3 to insert");
}

void test_rule2_error_handling_respond_to_failure()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // No errors initially
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.error_code, 0, "Rule 2: no error initially");
    ASSERT_EQ(outputs.error_storage, 0, "Rule 2: error storage empty initially");
    
    // Introduce error in cup 1
    inputs.cup_error[0] = 0x01;  // Error flag 1
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.error_code, 0x01, "Rule 2: error_code reflects cup 1 error");
    ASSERT_EQ(outputs.last_error, 0, "Rule 2: last_error stores previous error_code (was 0)");
    ASSERT_EQ(outputs.error_storage, 0x01, "Rule 2: error_storage accumulates error");
}

void test_rule2_error_handling_return_to_normal()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    state.prev_error_code = 0x01;  // Was an error
    state.error_storage = 0x01;    // Has history
    
    // Error clears
    inputs.cup_error[0] = 0;
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.error_code, 0, "Rule 2: error_code clears");
    ASSERT_EQ(outputs.last_error, 0x01, "Rule 2: last_error stores cleared error code");
    ASSERT_EQ(outputs.error_storage, 0x01, "Rule 2: error_storage retains history");
}

void test_rule2_multiple_cup_errors_combined()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // Errors in multiple cups
    inputs.cup_error[0] = 0x01;
    inputs.cup_error[1] = 0x02;
    inputs.cup_error[2] = 0x04;
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.error_code, 0x07, "Rule 2: error_code is OR of all cup errors");
}

void test_rule3_steady_state_enforcement()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // Cup 1: steady, user wants insert, but currently withdrawn
    inputs.cup_control[0] = true;   // User: insert
    inputs.cup_inserted[0] = false; // Current: withdrawn
    inputs.cup_steady[0] = true;    // Steady state
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.cup_requested_state[0], true, "Rule 3: steady state mismatch forces to control");
}

void test_rule3_no_enforcement_when_transient()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // Cup 1: transient (not steady), user wants insert, currently withdrawn
    inputs.cup_control[0] = true;   // User: insert
    inputs.cup_inserted[0] = false; // Current: withdrawn
    inputs.cup_steady[0] = false;   // Transient state
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.cup_requested_state[0], true, "Rule 3: non-steady state also follows control");
}

void test_rule4_active_cup_selection()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // No cups inserted initially
    inputs.cup_inserted[0] = false;
    inputs.cup_inserted[1] = false;
    inputs.cup_inserted[2] = false;
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.active_cup, 1, "Rule 4: no inserted cups, retain previous (was 1)");
    
    // Insert cup 2
    inputs.cup_inserted[1] = true;
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.active_cup, 2, "Rule 4: cup 2 inserted, active_cup = 2");
    
    // Insert cup 1 (higher priority from ion source)
    inputs.cup_inserted[0] = true;
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.active_cup, 1, "Rule 4: cup 1 inserted, active_cup = 1 (higher priority)");
    
    // Withdraw cup 1, cup 2 remains
    inputs.cup_inserted[0] = false;
    highLevelCtrlTick(&inputs, &state, &outputs);
    ASSERT_EQ(outputs.active_cup, 2, "Rule 4: cup 1 withdrawn, active_cup = 2 (next in priority)");
}

void test_rule1_priority_over_rule3()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // Setup: Rule 1 and Rule 3 conflict on cup 3
    inputs.external_inhibition = true;   // Rule 1: insert cup 3
    inputs.cup_control[2] = false;       // User: withdraw cup 3 (Rule 3 would enforce)
    inputs.cup_inserted[2] = false;      // Not inserted
    inputs.cup_steady[2] = true;         // Steady, so Rule 3 wants to enforce cup_control
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.cup_requested_state[2], true, "Rule 1 priority: Rule 1 (inhibit) overrides Rule 3");
}

void test_installed_cups_parameter()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state;
    HighLevelCtrlOutputs outputs;
    
    init_inputs_zero(&inputs);
    init_state_default(&state);
    
    // Test with only 1 cup installed
    inputs.installed_cups = 1;
    inputs.external_inhibition = true;
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    // Only cup 0 (cup #1) should be forced
    ASSERT_EQ(outputs.cup_requested_state[0], true, "With 1 cup, inhibition forces cup 0");
    
    // Test with 2 cups installed
    init_inputs_zero(&inputs);
    init_state_default(&state);
    inputs.installed_cups = 2;
    inputs.external_inhibition = true;
    
    highLevelCtrlTick(&inputs, &state, &outputs);
    
    ASSERT_EQ(outputs.cup_requested_state[1], true, "With 2 cups, inhibition forces cup 1 (last)");
}

void test_determinism_repeated_ticks()
{
    HighLevelCtrlInputs inputs;
    HighLevelCtrlState state1, state2;
    HighLevelCtrlOutputs outputs1, outputs2;
    
    init_inputs_zero(&inputs);
    init_state_default(&state1);
    memcpy(&state2, &state1, sizeof(HighLevelCtrlState));
    
    // Run first tick
    highLevelCtrlTick(&inputs, &state1, &outputs1);
    
    // Copy state and run second tick with same inputs
    memcpy(&state2, &state1, sizeof(HighLevelCtrlState));
    highLevelCtrlTick(&inputs, &state2, &outputs2);
    
    ASSERT_EQ(outputs1.active_cup, outputs2.active_cup, "Determinism: active_cup same after repeated ticks");
    ASSERT_EQ(outputs1.error_code, outputs2.error_code, "Determinism: error_code same after repeated ticks");
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main(void)
{
    printf("========================================\n");
    printf("HighLevelCtrl Unit Tests\n");
    printf("========================================\n\n");
    
    test_rule1_external_inhibition_locks_last_cup();
    test_rule1_inhibition_only_affects_last_cup();
    test_rule2_error_handling_respond_to_failure();
    test_rule2_error_handling_return_to_normal();
    test_rule2_multiple_cup_errors_combined();
    test_rule3_steady_state_enforcement();
    test_rule3_no_enforcement_when_transient();
    test_rule4_active_cup_selection();
    test_rule1_priority_over_rule3();
    test_installed_cups_parameter();
    test_determinism_repeated_ticks();
    
    printf("\n========================================\n");
    printf("Test Results: %d/%d passed\n", test_passed, test_count);
    printf("Failed: %d\n", test_failed);
    printf("========================================\n");
    
    return test_failed > 0 ? 1 : 0;
}
