/// @file auxiliaryFSMs.h
/// Auxiliary FSM logic for Faraday cup mechanisms.
/// Pure domain logic without hardware dependencies.

#ifndef AUXILIARY_FSMS_H
#define AUXILIARY_FSMS_H

#include <stdbool.h>
#include <stdint.h>

#define AUXILIARY_FSMS_MAX_CUPS 3u

#define AUXILIARY_FSM_CUP_TYPE_PNEUMATIC 0u
#define AUXILIARY_FSM_CUP_TYPE_MOTOR     1u

#define AUXILIARY_FSM_ERROR_TIMEOUT_INSERT     0x0001u
#define AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW   0x0002u
#define AUXILIARY_FSM_ERROR_INVALID_SWITCHES   0x0004u
#define AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG 0x0008u

typedef struct {
    uint16_t installed_cups;
    bool external_inhibition;

    uint16_t cup_type[AUXILIARY_FSMS_MAX_CUPS];

    uint16_t time_limit_inserting_ms[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t time_limit_withdrawing_ms[AUXILIARY_FSMS_MAX_CUPS];

    bool cup_requested_state[AUXILIARY_FSMS_MAX_CUPS];

    bool cup_switch[AUXILIARY_FSMS_MAX_CUPS];
    bool cup_switch1[AUXILIARY_FSMS_MAX_CUPS];
    bool cup_switch2[AUXILIARY_FSMS_MAX_CUPS];
} AuxiliaryFSMsInputs;

typedef struct {
    bool actuator_control[AUXILIARY_FSMS_MAX_CUPS];

    bool cup_inserted[AUXILIARY_FSMS_MAX_CUPS];
    bool cup_steady[AUXILIARY_FSMS_MAX_CUPS];

    uint16_t cup_error[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t cup_last_error[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t cup_error_storage[AUXILIARY_FSMS_MAX_CUPS];
} AuxiliaryFSMsOutputs;

typedef struct {
    uint16_t transition_elapsed_ms[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t prev_error[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t error_storage[AUXILIARY_FSMS_MAX_CUPS];
} AuxiliaryFSMsState;

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *inputs,
                       AuxiliaryFSMsState *state,
                       AuxiliaryFSMsOutputs *outputs,
                       uint16_t tick_period_ms);

#endif // AUXILIARY_FSMS_H
