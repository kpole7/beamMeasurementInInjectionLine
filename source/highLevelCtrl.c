/// @file highLevelCtrl.c
/// Implementation of high-level control logic for Faraday cup management.
/// Implements Rules 1-4 from documentation/tasks.md.

#include "highLevelCtrl.h"
#include <string.h>

//==============================================================================
// Internal helper functions
//==============================================================================

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

void highLevelCtrlTick(const HighLevelCtrlInputs *Inputs,
                       HighLevelCtrlState *State,
                       HighLevelCtrlOutputs *Outputs)
{
    // Validate inputs
    if (!Inputs || !State || !Outputs) {
        return;
    }
    
    // Initialize outputs with safe defaults
    memset(Outputs, 0, sizeof(HighLevelCtrlOutputs));
    
    for (uint16_t i = 0; i < MAX_CUPS; i++) {
        if (Inputs->external_inhibition && (CUP_TYPE_PNEUMATIC_WITH_LOCK == Inputs->cup_type[i])) {
            Outputs->cup_requested_state[i] = true;  // Force insert if external inhibition is active for pneumatic with lock
        } else {
            Outputs->cup_requested_state[i] = Inputs->cup_control[i];
        }
    }

    //--------------------------------------------------------------------------
    // Handle error state changes
    //--------------------------------------------------------------------------
    uint16_t new_error_code = calculateErrorCode(Inputs);
    
    if (new_error_code != 0) {
        // - backup ErrorCode in LastError register
        // - update ErrorCode register
        // - update ErrorStorage register
        Outputs->last_error = State->prev_error_code;
        Outputs->error_code = new_error_code;
        State->error_storage |= new_error_code;  // Accumulate errors
    }
    
    // Always output current error_storage value
    Outputs->error_storage = State->error_storage;
    
    //--------------------------------------------------------------------------
    // Update persistent state for next tick
    //--------------------------------------------------------------------------
    State->prev_error_code = new_error_code;
    // state->retained_active_cup is updated above
}
