/// @file highLevelCtrl.h
/// High-level control logic for Faraday cup management.
/// This module implements Rules 1-4 from documentation/tasks.md.
/// Pure domain logic without hardware dependencies (no Pico SDK).

#ifndef HIGH_LEVEL_CTRL_H
#define HIGH_LEVEL_CTRL_H

#include <stdbool.h>
#include <stdint.h>

/// Maximum number of Faraday cups supported
#define MAX_CUPS 3

/// Inputs for a single tick of the HighLevelCtrl state machine.
typedef struct {
    /// Number of installed cups (1-3)
    uint16_t installed_cups;
    
    /// External inhibition signal: true = lock beam immediately
    bool external_inhibition;
    
    /// User control request for each cup: true = insert, false = withdraw
    bool cup_control[MAX_CUPS];
    
    /// Error status for each cup (bitmap)
    uint16_t cup_error[MAX_CUPS];
    
    /// Steady state indicator for each cup: true = steady, false = transient
    bool cup_steady[MAX_CUPS];
    
    /// Inserted status from AuxiliaryFSM for each cup: true = inserted, false = withdrawn
    bool cup_inserted[MAX_CUPS];
} HighLevelCtrlInputs;

/// Outputs produced by a single tick of the HighLevelCtrl state machine.
typedef struct {
    /// Error code for the entire device (bitmap of all cup errors)
    uint16_t error_code;
    
    /// Code of the last error that occurred
    uint16_t last_error;
    
    /// Bitwise OR of all errors that have occurred (can be reset via Modbus)
    uint16_t error_storage;
    
    /// The first inserted Faraday cup as seen from ion source side (1-3).
    /// If none is inserted, this field is not updated.
    uint16_t active_cup;
    
    /// Requested state for each cup to AuxiliaryFSM: true = insert, false = withdraw
    bool cup_requested_state[MAX_CUPS];
} HighLevelCtrlOutputs;

/// Internal state for HighLevelCtrl that must persist between ticks.
typedef struct {
    /// Previous value of error_code (used to detect transitions)
    uint16_t prev_error_code;
    
    /// Retained active cup (does not change when no cup is inserted)
    uint16_t retained_active_cup;
    
    /// Accumulated error history (bitwise OR of all errors seen).
    /// Persists until explicitly cleared (e.g., via Modbus adapter).
    uint16_t error_storage;
} HighLevelCtrlState;

/// Execute one tick of HighLevelCtrl logic at 400 Hz frequency.
/// Implements Rules 1-4 in priority order: Rule 1 > Rule 2 > Rule 3 > Rule 4.
///
/// @param inputs  Pointer to current inputs from hardware and AuxiliaryFSMs
/// @param state   Pointer to persistent state; updated after this call
/// @param outputs Pointer to output structure; filled with results
void highLevelCtrlTick(const HighLevelCtrlInputs *inputs,
                       HighLevelCtrlState *state,
                       HighLevelCtrlOutputs *outputs);

#endif // HIGH_LEVEL_CTRL_H
