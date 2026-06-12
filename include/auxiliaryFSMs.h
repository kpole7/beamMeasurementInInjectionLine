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

typedef enum {
    PNEUMATIC_FSM_STATE_UNSPECIFIED = 0,
    PNEUMATIC_FSM_STATE_EXTRACTED,  // stable
    PNEUMATIC_FSM_STATE_INSERTING,
    PNEUMATIC_FSM_STATE_INSERTED,   // stable
    PNEUMATIC_FSM_STATE_WITHDRAWING,
    PNEUMATIC_FSM_STATE_ERROR
} PneumaticFsmStateEnum;

typedef enum {
    MOTOR_FSM_STATE_UNSPECIFIED = 0,
    MOTOR_FSM_STATE_EXTRACTED,  // stable
    MOTOR_FSM_STATE_INSERTING,
    MOTOR_FSM_STATE_INSERTED_PRE_BRAKING,
    MOTOR_FSM_STATE_INSERTED_BRAKING,
    MOTOR_FSM_STATE_INSERTED,   // stable
    MOTOR_FSM_STATE_WITHDRAWING,
    MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING,
    MOTOR_FSM_STATE_WITHDRAWING_BRAKING,
    MOTOR_FSM_STATE_ERROR
} MotorFsmStateEnum;

typedef struct {
    uint16_t installed_cups;
    bool external_inhibition;

    uint16_t cup_type[AUXILIARY_FSMS_MAX_CUPS];

    uint16_t time_limit_inserting_ms[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t time_limit_withdrawing_ms[AUXILIARY_FSMS_MAX_CUPS];

    bool cup_requested_state[AUXILIARY_FSMS_MAX_CUPS];

    bool cup_switch[AUXILIARY_FSMS_MAX_CUPS];
    bool cup_switch_a[AUXILIARY_FSMS_MAX_CUPS];
    bool cup_switch_b[AUXILIARY_FSMS_MAX_CUPS];
} AuxiliaryFSMsInputs;

typedef struct {
    bool actuator_control[AUXILIARY_FSMS_MAX_CUPS];
    bool actuator_insert[AUXILIARY_FSMS_MAX_CUPS];
    bool actuator_withdraw[AUXILIARY_FSMS_MAX_CUPS];
    bool actuator_brake[AUXILIARY_FSMS_MAX_CUPS];

    uint16_t cup_error[AUXILIARY_FSMS_MAX_CUPS];
} AuxiliaryFSMsOutputs;

typedef struct {
    PneumaticFsmStateEnum pneumatic_fsm_state[AUXILIARY_FSMS_MAX_CUPS];
    MotorFsmStateEnum motor_fsm_state[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t transition_elapsed[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t pre_braking_elapsed[AUXILIARY_FSMS_MAX_CUPS];
    uint16_t braking_elapsed[AUXILIARY_FSMS_MAX_CUPS];
} AuxiliaryFSMsState;

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *inputs,
                       AuxiliaryFSMsState *FsmState,
                       AuxiliaryFSMsOutputs *outputs);

#endif // AUXILIARY_FSMS_H
