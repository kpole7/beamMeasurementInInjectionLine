/// @file highLevelCtrl.c
/// Implementation of high-level control logic for Faraday cup management.
/// Implements Rules 1-4 from documentation/tasks.md.

#include "highLevelCtrl.h"
#include <string.h>

//==============================================================================
// Internal helper functions
//==============================================================================

/// Determine which cup is the first inserted, viewed from ion source.
/// @param inputs Pointer to inputs
/// @return Cup index (1-3) if found, or 0 if none inserted
static uint16_t findFirstInsertedCup(const HighLevelCtrlInputs *inputs)
{
    for (uint16_t i = 0; i < inputs->installed_cups; i++) {
        if (inputs->cup_inserted[i]) {
            return i + 1;  // Cups are numbered 1-3, not 0-2
        }
    }
    return 0;  // No cup inserted
}

/// Calculate error_code as bitwise OR of all cup errors.
static uint16_t calculateErrorCode(const HighLevelCtrlInputs *inputs)
{
    uint16_t error_code = 0;
    for (uint16_t i = 0; i < inputs->installed_cups; i++) {
        error_code |= inputs->cup_error[i];
    }
    return error_code;
}

//==============================================================================
// Main tick function
//==============================================================================

void highLevelCtrlTick(const HighLevelCtrlInputs *inputs,
                       HighLevelCtrlState *state,
                       HighLevelCtrlOutputs *outputs)
{
    // Validate inputs
    if (!inputs || !state || !outputs) {
        return;
    }
    
    // Initialize outputs with safe defaults
    memset(outputs, 0, sizeof(HighLevelCtrlOutputs));
    
    //--------------------------------------------------------------------------
    // Rule 2: Handle error state changes
    //--------------------------------------------------------------------------
    uint16_t new_error_code = calculateErrorCode(inputs);
    
    if (new_error_code != 0) {
        // "Respond to failure" action:
        // - backup ErrorCode in LastError register
        // - update ErrorCode register
        // - update ErrorStorage register
        outputs->last_error = state->prev_error_code;
        outputs->error_code = new_error_code;
        state->error_storage |= new_error_code;  // Accumulate errors
    } else {
        // "Return to normal" action:
        // - if ErrorCode!=0, then backup ErrorCode in LastError
        // - update ErrorCode register
        if (state->prev_error_code != 0) {
            outputs->last_error = state->prev_error_code;
        }
        outputs->error_code = 0;
        // error_storage is NOT cleared; it retains history
    }
    
    // Always output current error_storage value
    outputs->error_storage = state->error_storage;
    
    //--------------------------------------------------------------------------
    // Rule 4: Determine active cup (first inserted from ion source)
    //--------------------------------------------------------------------------
    uint16_t first_inserted = findFirstInsertedCup(inputs);
    
    if (first_inserted != 0) {
        // At least one cup is inserted
        outputs->active_cup = first_inserted;
        state->retained_active_cup = first_inserted;
    } else {
        // No cup inserted: retain previous value
        outputs->active_cup = state->retained_active_cup;
    }
    
    //--------------------------------------------------------------------------
    // Rule 3: Set requested state for each cup based on steady state
    //--------------------------------------------------------------------------
    for (uint16_t i = 0; i < inputs->installed_cups; i++) {
        if (inputs->cup_steady[i] && (inputs->cup_inserted[i] != inputs->cup_control[i])) {
            // Cup is steady but not in requested state: force to control
            outputs->cup_requested_state[i] = inputs->cup_control[i];
        } else {
            // Default: follow user's request
            outputs->cup_requested_state[i] = inputs->cup_control[i];
        }
    }
    
    //--------------------------------------------------------------------------
    // Rule 1: Handle external inhibition (highest priority)
    // Override any other rule for the last cup (closest to cyclotron)
    //--------------------------------------------------------------------------
    if (inputs->external_inhibition) {
        // Lock beam: insert the last cup
        uint16_t last_cup_index = inputs->installed_cups - 1;
        outputs->cup_requested_state[last_cup_index] = true;
    }
    
    //--------------------------------------------------------------------------
    // Update persistent state for next tick
    //--------------------------------------------------------------------------
    state->prev_error_code = new_error_code;
    // state->retained_active_cup is updated above
}
