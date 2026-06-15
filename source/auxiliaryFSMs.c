/// @file auxiliaryFSMs.c
/// Auxiliary FSM logic for Faraday cup mechanisms.

#include "auxiliaryFSMs.h"
#include <limits.h>
#include <string.h>


#define PRE_BRAKING_TIME_IN_TICKS 5u
#define BRAKING_TIME_IN_TICKS 400u


static uint16_t clampInstalledCups(uint16_t value)
{
    if (value < 1u) {
        return 1u;
    }
    if (value > AUXILIARY_FSMS_MAX_CUPS) {
        return AUXILIARY_FSMS_MAX_CUPS;
    }
    return value;
}

static inline void saturatingIncreaseU16(uint16_t *a) {
    if (*a < UINT16_MAX) {
        (*a)++;
    }
}

static void pneumaticFsmUnspecified(bool requested, bool sw, PneumaticFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticFsmExtracted(bool requested, bool sw, PneumaticFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticFsmInserting(bool requested, bool sw, PneumaticFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticFsmInserted(bool requested, bool sw, PneumaticFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticFsmWithdrawing(bool requested, bool sw, PneumaticFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void evaluatePneumaticCup(bool requested, bool sw, PneumaticFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {
    switch (*pneumatic_fsm_state) {
        case PNEUMATIC_FSM_STATE_UNSPECIFIED:
            pneumaticFsmUnspecified(requested, sw, pneumatic_fsm_state, actuator, error);
            break;
        case PNEUMATIC_FSM_STATE_EXTRACTED:
            pneumaticFsmExtracted(requested, sw, pneumatic_fsm_state, actuator, error);
            break;
        case PNEUMATIC_FSM_STATE_INSERTING:
            pneumaticFsmInserting(requested, sw, pneumatic_fsm_state, actuator, error);
            break;
        case PNEUMATIC_FSM_STATE_INSERTED:
            pneumaticFsmInserted(requested, sw, pneumatic_fsm_state, actuator, error);
            break;
        case PNEUMATIC_FSM_STATE_WITHDRAWING:
            pneumaticFsmWithdrawing(requested, sw, pneumatic_fsm_state, actuator, error);
            break;
        default:
            *error |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;

#if 0
    *inserted = sw;
    *actuator = requested;

    if (requested) {
        *steady = sw;
    } else {
        *steady = !sw;
    }

    *transient = !(*steady);
#endif
}

static void pneumaticWithLockFsmUnspecified(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {
    // check whether all is in the initial state
    if (inhibit) {

    }
}

static void pneumaticWithLockFsmLockedInserted(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticWithLockFsmLockedInserting(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticWithLockFsmExtracted(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticWithLockFsmInserting(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticWithLockFsmInserted(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void pneumaticWithLockFsmWithdrawing(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_fsm_state, bool *actuator, uint16_t *error) {

}

static void evaluatePneumaticWithLockCup(bool inhibit, bool requested, bool sw, PneumaticWithLockFsmStateEnum *pneumatic_with_lock_fsm_state, 
    bool *actuator, uint16_t *error) 
    {
    switch (*pneumatic_with_lock_fsm_state) {
        case PNEUMATIC_WITH_LOCK_FSM_STATE_UNSPECIFIED:
            pneumaticWithLockFsmUnspecified(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTED:
            pneumaticWithLockFsmLockedInserted(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTING:
            pneumaticWithLockFsmLockedInserting(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED:
            pneumaticWithLockFsmExtracted(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING:
            pneumaticWithLockFsmInserting(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED:
            pneumaticWithLockFsmInserted(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING:
            pneumaticWithLockFsmWithdrawing(inhibit, requested, sw, pneumatic_with_lock_fsm_state, actuator, error);
            break;
        default:
            *error |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;

#if 0
    *inserted = sw;
    *actuator = requested;

    if (requested) {
        *steady = sw;
    } else {
        *steady = !sw;
    }

    *transient = !(*steady);
#endif
}

static void motorFsmUnspecified(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmExtracted(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmInserting(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmInsertedPreBraking(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmInsertedBraking(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmInserted(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmWithdrawing(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmWithdrawingPreBraking(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

static void motorFsmWithdrawingBraking(bool requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *actuator_insert, bool *actuator_withdraw, bool *actuator_brake, uint16_t *error)
{

}

/// @brief 
/// @param requested  the input data corresponding to the Modbus coil CupNRequestedState
/// @param sw_a       the input data corresponding to the Modbus coil CupNSwitchA
/// @param sw_b       the input data corresponding to the Modbus coil CupNSwitchB
/// @param motor_fsm_state  the state variable corresponding to the Modbus register CupNFsmState
/// @param actuator_insert     the output data corresponding to the Modbus coil Actuator3ControlIn
/// @param actuator_withdraw   the output data corresponding to the Modbus coil Actuator3ControlOut
/// @param actuator_brake      the output data corresponding to the Modbus coil Actuator3ControlBrake
static void evaluateMotorCup(bool requested,
                             bool sw_a,
                             bool sw_b,
                             bool pre_braking_elapsed,
                             bool braking_elapsed,
                             MotorFsmStateEnum *motor_fsm_state,
                             bool *actuator_insert,
                             bool *actuator_withdraw,
                             bool *actuator_brake,
                             uint16_t *error)
{
    switch (*motor_fsm_state) {
        case MOTOR_FSM_STATE_UNSPECIFIED:
            motorFsmUnspecified(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_EXTRACTED:
            motorFsmExtracted(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_INSERTING:
            motorFsmInserting(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_INSERTED_PRE_BRAKING:
            motorFsmInsertedPreBraking(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_INSERTED_BRAKING:
            motorFsmInsertedBraking(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_INSERTED:
            motorFsmInserted(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING:
            motorFsmWithdrawing(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING:
            motorFsmWithdrawingPreBraking(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING_BRAKING:
            motorFsmWithdrawingBraking(requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, actuator_insert, actuator_withdraw, actuator_brake, error);
            break;
        default:
            *error |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;

#if 0
    bool is_withdrawn = sw_a && !sw_b;
    bool is_inserted = !sw_a && sw_b;
    bool is_moving = !sw_a && !sw_b;
    *invalid = sw_a && sw_b;
    *inserted = is_inserted;
    *actuator_insert = requested && !is_inserted;

    if (*invalid) {
        *steady = false;
        *transient = false;
        return;
    }

    if (requested) {
        *steady = is_inserted;
    } else {
        *steady = is_withdrawn;
    }

    *transient = is_moving || !(*steady);
#endif
}

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *inputs,
                       AuxiliaryFSMsState *FsmState,
                       AuxiliaryFSMsOutputs *outputs)
{
    if (!inputs || !FsmState || !outputs) {
        return;
    }

    memset(outputs, 0, sizeof(AuxiliaryFSMsOutputs));

    uint16_t installed_cups = clampInstalledCups(inputs->installed_cups);

    for (uint16_t cup = 0; cup < installed_cups; cup++) {
        bool inhibit = inputs->external_inhibition;
        bool requested = inputs->cup_requested_state[cup];
        PneumaticFsmStateEnum pneumatic_fsm_state = FsmState->pneumatic_fsm_state[cup];
        PneumaticWithLockFsmStateEnum pneumatic_with_lock_fsm_state = FsmState->pneumatic_with_lock_fsm_state[cup];
        MotorFsmStateEnum motor_fsm_state = FsmState->motor_fsm_state[cup];
        bool pre_braking_time_exceeded = false;
        bool braking_time_exceeded = false;
        bool actuator = false;
        bool actuator_insert = false;
        bool actuator_withdraw = false;
        bool actuator_brake = false;
        uint16_t error = 0u;

        if (inputs->cup_type[cup] == AUXILIARY_FSM_CUP_TYPE_PNEUMATIC) {
            evaluatePneumaticCup(requested,
                                 inputs->cup_switch[cup],
                                 &pneumatic_fsm_state,
                                 &actuator,
                                 &error);
            if (PNEUMATIC_FSM_STATE_INSERTING == pneumatic_fsm_state){
                uint16_t effective_limit = (inputs->time_limit_inserting_ms[cup] == 0u) ? 1u : inputs->time_limit_inserting_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
                }
            } else if (PNEUMATIC_FSM_STATE_WITHDRAWING == pneumatic_fsm_state) {
                uint16_t effective_limit = (inputs->time_limit_withdrawing_ms[cup] == 0u) ? 1u : inputs->time_limit_withdrawing_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
                }
            } else {
                FsmState->transition_elapsed[cup] = 0u;
            }
        } else if (inputs->cup_type[cup] == AUXILIARY_FSM_CUP_TYPE_PNEUMATIC_WITH_LOCK) {
            evaluatePneumaticWithLockCup(inhibit,
                                         requested,
                                         inputs->cup_switch[cup],
                                         &pneumatic_with_lock_fsm_state,
                                         &actuator,
                                         &error);
            if (PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTING == pneumatic_with_lock_fsm_state){
                uint16_t effective_limit = (inputs->time_limit_inserting_ms[cup] == 0u) ? 1u : inputs->time_limit_inserting_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
                }
            } else if (PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING == pneumatic_with_lock_fsm_state){
                uint16_t effective_limit = (inputs->time_limit_inserting_ms[cup] == 0u) ? 1u : inputs->time_limit_inserting_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
                }
            } else if (PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING == pneumatic_with_lock_fsm_state) {
                uint16_t effective_limit = (inputs->time_limit_withdrawing_ms[cup] == 0u) ? 1u : inputs->time_limit_withdrawing_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
                }
            } else {
                FsmState->transition_elapsed[cup] = 0u;
            }
        } else if (inputs->cup_type[cup] == AUXILIARY_FSM_CUP_TYPE_MOTOR) {

            if ((MOTOR_FSM_STATE_INSERTED_PRE_BRAKING == motor_fsm_state) || (MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING == motor_fsm_state)){
                saturatingIncreaseU16(&FsmState->pre_braking_elapsed[cup]);
                if (FsmState->pre_braking_elapsed[cup] > PRE_BRAKING_TIME_IN_TICKS) {
                    pre_braking_time_exceeded = true;
                }
            } else {
                FsmState->pre_braking_elapsed[cup] = 0u;
            }
            if ((MOTOR_FSM_STATE_INSERTED_BRAKING == motor_fsm_state) || (MOTOR_FSM_STATE_WITHDRAWING_BRAKING == motor_fsm_state)){
                saturatingIncreaseU16(&FsmState->braking_elapsed[cup]);
                if (FsmState->braking_elapsed[cup] > BRAKING_TIME_IN_TICKS) {
                    braking_time_exceeded = true;
                }
            } else {
                FsmState->braking_elapsed[cup] = 0u;
            }

            evaluateMotorCup(requested,
                             inputs->cup_switch_a[cup],
                             inputs->cup_switch_b[cup],
                             pre_braking_time_exceeded,
                             braking_time_exceeded,
                             &motor_fsm_state,
                             &actuator_insert,
                             &actuator_withdraw,
                             &actuator_brake,
                             &error);

            if (MOTOR_FSM_STATE_INSERTING == motor_fsm_state){
                uint16_t effective_limit = (inputs->time_limit_inserting_ms[cup] == 0u) ? 1u : inputs->time_limit_inserting_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
                }
            } else if (MOTOR_FSM_STATE_WITHDRAWING == motor_fsm_state) {
                uint16_t effective_limit = (inputs->time_limit_withdrawing_ms[cup] == 0u) ? 1u : inputs->time_limit_withdrawing_ms[cup];
                saturatingIncreaseU16(&FsmState->transition_elapsed[cup]);
                if (FsmState->transition_elapsed[cup] > effective_limit) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
                }
            } else {
                FsmState->transition_elapsed[cup] = 0u;
            }
        } else {
            error |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            pneumatic_fsm_state = MOTOR_FSM_STATE_ERROR;
            motor_fsm_state = MOTOR_FSM_STATE_ERROR;
            actuator = false;
            actuator_insert = false;
            actuator_withdraw = false;
            actuator_brake = false;
        }

        outputs->actuator_insert[cup] = actuator_insert;
        outputs->actuator_withdraw[cup] = actuator_withdraw;
        outputs->actuator_brake[cup] = actuator_brake;
        outputs->cup_error[cup] = error;
    }
}
