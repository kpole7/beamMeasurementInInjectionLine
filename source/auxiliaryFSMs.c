/// @file auxiliaryFSMs.c
/// Auxiliary FSM logic for Faraday cup mechanisms.

#include "auxiliaryFSMs.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>


#define PRE_BRAKING_TIME_IN_TICKS 5u
#define BRAKING_TIME_IN_TICKS 400u
#define PAUSE_AFTER_LOCK_TIME_IN_TICKS 500u
#define PAUSE_AFTER_UNLOCK_TIME_IN_TICKS 250u

static uint16_t clampInstalledCups(uint16_t Value)
{
    if (Value < 1u) {
        return 1u;
    }
    if (Value > AUXILIARY_FSMS_MAX_CUPS) {
        return AUXILIARY_FSMS_MAX_CUPS;
    }
    return Value;
}

static inline void saturatingIncreaseU16(uint16_t *A) {
    if (*A < UINT16_MAX) {
        (*A)++;
    }
}

static void pneumaticFsmBooted(bool Requested, bool Switch, PneumaticFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) {
    if (!Requested || !Switch) {
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_INSERTED;
    printf("New state=%d; Requested=%d, sw=%d; file %s, line %d\n", *PneumaticFsmStatePtr, Requested, Switch, __FILE__, __LINE__);
}

static void pneumaticFsmExtracted(bool Requested, bool Switch, PneumaticFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) {
    if (Switch) {
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    if (Requested) {
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_INSERTING;
        printf("New state=%d; requested=%d, sw=%d; file %s, line %d\n", *PneumaticFsmStatePtr, Requested, Switch, __FILE__, __LINE__);
    }
}

static void pneumaticFsmInserting(bool Requested, bool Switch, PneumaticFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) {
    if (Switch) {
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_INSERTED;
        printf("New state=%d; requested=%d, sw=%d; file %s, line %d\n", *PneumaticFsmStatePtr, Requested, Switch, __FILE__, __LINE__);
    }
}

static void pneumaticFsmInserted(bool Requested, bool Switch, PneumaticFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) {
    if (!Switch) {
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    if (!Requested) {
        *ActuatorPtr = false;
        *TriggerPtr = true;
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_WITHDRAWING;
        printf("New state=%d; requested=%d, sw=%d; file %s, line %d\n", *PneumaticFsmStatePtr, Requested, Switch, __FILE__, __LINE__);
    }
}

static void pneumaticFsmWithdrawing(bool Requested, bool Switch, PneumaticFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) {
    if (!Switch) {
        *PneumaticFsmStatePtr = PNEUMATIC_FSM_STATE_EXTRACTED;
        printf("New state=%d; requested=%d, sw=%d; file %s, line %d\n", *PneumaticFsmStatePtr, Requested, Switch, __FILE__, __LINE__);
    }
}

static void evaluatePneumaticCup(bool Requested, bool Switch, PneumaticFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) {
    switch (*PneumaticFsmStatePtr) {
        case PNEUMATIC_FSM_STATE_BOOTED:
            pneumaticFsmBooted(Requested, Switch, PneumaticFsmStatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_EXTRACTED:
            pneumaticFsmExtracted(Requested, Switch, PneumaticFsmStatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_INSERTING:
            pneumaticFsmInserting(Requested, Switch, PneumaticFsmStatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_INSERTED:
            pneumaticFsmInserted(Requested, Switch, PneumaticFsmStatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_WITHDRAWING:
            pneumaticFsmWithdrawing(Requested, Switch, PneumaticFsmStatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        default:
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;
}

static void pneumaticWithLockFsmBooted(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (Inhibit) {

    }
    else{
        if (!Requested || !Switch) {
            *PneumaticFsmStatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR;
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
            return;
        }
        *PneumaticFsmStatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED;
        printf("New state=%d; requested=%d, sw=%d; file %s, line %d\n", *PneumaticFsmStatePtr, Requested, Switch, __FILE__, __LINE__);
    }
}

static void pneumaticWithLockFsmExtracted(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void pneumaticWithLockFsmInserting(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void pneumaticWithLockFsmInserted(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void pneumaticWithLockFsmWithdrawing(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void pneumaticWithLockFsmPauseAfterLock(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void pneumaticWithLockFsmLockedInserted(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void pneumaticWithLockFsmPauseAfterUnlock(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *PneumaticFsmStatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {

}

static void evaluatePneumaticWithLockCup(bool Inhibit, bool Requested, bool Switch, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *pneumatic_with_lock_fsm_state, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    switch (*pneumatic_with_lock_fsm_state) {
        case PNEUMATIC_WITH_LOCK_FSM_STATE_BOOTED:
            pneumaticWithLockFsmBooted(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED:
            pneumaticWithLockFsmExtracted(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING:
            pneumaticWithLockFsmInserting(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED:
            pneumaticWithLockFsmInserted(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING:
            pneumaticWithLockFsmWithdrawing(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK:
            pneumaticWithLockFsmPauseAfterLock(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTED:
            pneumaticWithLockFsmLockedInserted(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK:
            pneumaticWithLockFsmPauseAfterUnlock(Inhibit, Requested, Switch, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                pneumatic_with_lock_fsm_state, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        default:
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;
}

static void motorFsmBooted(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmExtracted(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmInserting(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmInsertedPreBraking(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmInsertedBraking(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmInserted(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmWithdrawing(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmWithdrawingPreBraking(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

static void motorFsmWithdrawingBraking(bool Requested, bool sw_a, bool sw_b, bool pre_braking_elapsed, bool braking_elapsed, MotorFsmStateEnum *motor_fsm_state, 
    bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, bool *TriggerInsertPtr, bool *TriggerWithdraw, bool trigger_brake, uint16_t *ErrorPtr)
{

}

/// @brief 
/// @param Requested  the input data corresponding to the Modbus coil CupNRequestedState
/// @param sw_a       the input data corresponding to the Modbus coil CupNSwitchA
/// @param sw_b       the input data corresponding to the Modbus coil CupNSwitchB
/// @param motor_fsm_state  the state variable corresponding to the Modbus register CupNFsmState
/// @param ActuatorInsertPtr     the output data corresponding to the Modbus coil Actuator3ControlIn
/// @param actuator_withdraw   the output data corresponding to the Modbus coil Actuator3ControlOut
/// @param actuator_brake      the output data corresponding to the Modbus coil Actuator3ControlBrake
static void evaluateMotorCup(bool Requested,
                             bool sw_a,
                             bool sw_b,
                             bool pre_braking_elapsed,
                             bool braking_elapsed,
                             MotorFsmStateEnum *motor_fsm_state,
                             bool *ActuatorInsertPtr,
                             bool *ActuatorWithdrawPtr,
                             bool *ActuatorBrakePtr,
                             bool *TriggerInsertPtr,
                             bool *TriggerWithdrawPtr,
                             bool *TriggerBrakePtr,
                             uint16_t *ErrorPtr)
{
    switch (*motor_fsm_state) {
        case MOTOR_FSM_STATE_BOOTED:
            motorFsmBooted(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_EXTRACTED:
            motorFsmExtracted(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTING:
            motorFsmInserting(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTED_PRE_BRAKING:
            motorFsmInsertedPreBraking(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTED_BRAKING:
            motorFsmInsertedBraking(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTED:
            motorFsmInserted(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING:
            motorFsmWithdrawing(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING:
            motorFsmWithdrawingPreBraking(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING_BRAKING:
            motorFsmWithdrawingBraking(Requested, sw_a, sw_b, pre_braking_elapsed, braking_elapsed, motor_fsm_state, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        default:
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;
}

void pneumaticFsmTick(uint16_t cup, 
                      const AuxiliaryFSMsInputs *InputsPtr,
                      AuxiliaryFSMsState *FsmStatePtr,
                      AuxiliaryFSMsOutputs *OutputsPtr)
{
    PneumaticFsmStateEnum PneumaticLocalState = FsmStatePtr->pneumatic_fsm_state[cup];
    bool Actuator = false;
    bool Trigger = false;
    uint16_t Error = 0u;

    if (InputsPtr->cup_type[cup] != AUXILIARY_FSM_CUP_TYPE_PNEUMATIC){
        return;
    }
    evaluatePneumaticCup(InputsPtr->cup_requested_state[cup],
                         InputsPtr->cup_switch[cup],
                         &PneumaticLocalState,
                         &Actuator,
                         &Trigger,
                         &Error);
    if (PneumaticLocalState == PNEUMATIC_FSM_STATE_INSERTING){
        uint16_t effective_limit = (InputsPtr->time_limit_inserting_ms[cup] == 0u) ? 1u : InputsPtr->time_limit_inserting_ms[cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[cup]);
        if (FsmStatePtr->transition_elapsed[cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
        }
    } else if (PneumaticLocalState == PNEUMATIC_FSM_STATE_WITHDRAWING) {
        uint16_t effective_limit = (InputsPtr->time_limit_withdrawing_ms[cup] == 0u) ? 1u : InputsPtr->time_limit_withdrawing_ms[cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[cup]);
        if (FsmStatePtr->transition_elapsed[cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
        }
    } else {
        FsmStatePtr->transition_elapsed[cup] = 0u;
    }

    FsmStatePtr->pneumatic_fsm_state[cup] = PneumaticLocalState;
    if (Trigger) {
        OutputsPtr->actuator_insert[cup] = Actuator;
    }
    OutputsPtr->trigger_insert[cup] = Trigger;
    OutputsPtr->cup_error[cup] = Error;
}

void pneumaticWithLockFsmTick(uint16_t Cup, 
                              const AuxiliaryFSMsInputs *InputsPtr,
                              AuxiliaryFSMsState *FsmStatePtr,
                              AuxiliaryFSMsOutputs *OutputsPtr)
{
    bool Inhibit = InputsPtr->external_inhibition;
    PneumaticWithLockFsmStateEnum pneumatic_with_lock_local_state = FsmStatePtr->pneumatic_with_lock_fsm_state[Cup];
    bool Actuator = false;
    bool Trigger = false;
    bool pause_after_lock_is_over = false;
    bool pause_after_unlock_is_over = false;
    uint16_t Error = 0u;

    if (InputsPtr->cup_type[Cup] != AUXILIARY_FSM_CUP_TYPE_PNEUMATIC_WITH_LOCK) {
        return;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK == pneumatic_with_lock_local_state) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_lock_elapsed[Cup]);
        if (FsmStatePtr->pause_after_lock_elapsed[Cup] > PAUSE_AFTER_LOCK_TIME_IN_TICKS) {
            pause_after_lock_is_over = true;
        }
    } else {
        FsmStatePtr->pause_after_lock_elapsed[Cup] = 0u;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK == pneumatic_with_lock_local_state) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_unlock_elapsed[Cup]);
        if (FsmStatePtr->pause_after_unlock_elapsed[Cup] > PAUSE_AFTER_UNLOCK_TIME_IN_TICKS) {
            pause_after_unlock_is_over = true;
        }
    } else {
        FsmStatePtr->pause_after_unlock_elapsed[Cup] = 0u;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING == pneumatic_with_lock_local_state){
        uint16_t effective_limit = (InputsPtr->time_limit_inserting_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_inserting_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
        }
    } else if (PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING == pneumatic_with_lock_local_state) {
        uint16_t effective_limit = (InputsPtr->time_limit_withdrawing_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_withdrawing_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
        }
    } else {
        FsmStatePtr->transition_elapsed[Cup] = 0u;
    }

    evaluatePneumaticWithLockCup(Inhibit,
                                 InputsPtr->cup_requested_state[Cup],
                                 InputsPtr->cup_switch[Cup],
                                 pause_after_lock_is_over,
                                 pause_after_unlock_is_over,
                                 &pneumatic_with_lock_local_state,
                                 &Actuator,
                                 &Trigger,
                                 &Error);
    FsmStatePtr->pneumatic_with_lock_fsm_state[Cup] = pneumatic_with_lock_local_state;
    if (Trigger) {
        OutputsPtr->actuator_insert[Cup] = Actuator;
    }
    OutputsPtr->trigger_insert[Cup] = Trigger;
    OutputsPtr->cup_error[Cup] = Error;
}

void motorFsmTick(uint16_t cup, 
                  const AuxiliaryFSMsInputs *InputsPtr,
                  AuxiliaryFSMsState *FsmStatePtr,
                  AuxiliaryFSMsOutputs *OutputsPtr)
{
    MotorFsmStateEnum MotorLocalState = FsmStatePtr->motor_fsm_state[cup];
    bool PreBrakingTimeExceeded = false;
    bool BrakingTimeExceeded = false;
    bool ActuatorInsert = false;
    bool ActuatorWithdraw = false;
    bool ActuatorBrake = false;
    bool TriggerInsert = false;
    bool TriggerWithdraw = false;
    bool TriggerBrake = false;
    uint16_t Error = 0u;

    if (InputsPtr->cup_type[cup] != AUXILIARY_FSM_CUP_TYPE_MOTOR) {
        return;
    }

    if ((MOTOR_FSM_STATE_INSERTED_PRE_BRAKING == MotorLocalState) || (MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING == MotorLocalState)){
        saturatingIncreaseU16(&FsmStatePtr->pre_braking_elapsed[cup]);
        if (FsmStatePtr->pre_braking_elapsed[cup] > PRE_BRAKING_TIME_IN_TICKS) {
            PreBrakingTimeExceeded = true;
        }
    } else {
        FsmStatePtr->pre_braking_elapsed[cup] = 0u;
    }
    if ((MOTOR_FSM_STATE_INSERTED_BRAKING == MotorLocalState) || (MOTOR_FSM_STATE_WITHDRAWING_BRAKING == MotorLocalState)){
        saturatingIncreaseU16(&FsmStatePtr->braking_elapsed[cup]);
        if (FsmStatePtr->braking_elapsed[cup] > BRAKING_TIME_IN_TICKS) {
            BrakingTimeExceeded = true;
        }
    } else {
        FsmStatePtr->braking_elapsed[cup] = 0u;
    }
    if (MOTOR_FSM_STATE_INSERTING == MotorLocalState){
        uint16_t effective_limit = (InputsPtr->time_limit_inserting_ms[cup] == 0u) ? 1u : InputsPtr->time_limit_inserting_ms[cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[cup]);
        if (FsmStatePtr->transition_elapsed[cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
        }
    } else if (MOTOR_FSM_STATE_WITHDRAWING == MotorLocalState) {
        uint16_t effective_limit = (InputsPtr->time_limit_withdrawing_ms[cup] == 0u) ? 1u : InputsPtr->time_limit_withdrawing_ms[cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[cup]);
        if (FsmStatePtr->transition_elapsed[cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
        }
    } else {
        FsmStatePtr->transition_elapsed[cup] = 0u;
    }
    evaluateMotorCup(InputsPtr->cup_requested_state[cup],
                     InputsPtr->cup_switch_a[cup],
                     InputsPtr->cup_switch_b[cup],
                     PreBrakingTimeExceeded,
                     BrakingTimeExceeded,
                     &MotorLocalState,
                     &ActuatorInsert,
                     &ActuatorWithdraw,
                     &ActuatorBrake,
                     &TriggerInsert,
                     &TriggerWithdraw,
                     &TriggerBrake,
                     &Error);

    FsmStatePtr->motor_fsm_state[cup] = MotorLocalState;
    if (TriggerInsert) {
        OutputsPtr->actuator_insert[cup] = ActuatorInsert;
    }
    OutputsPtr->trigger_insert[cup] = TriggerInsert;
    if (TriggerWithdraw) {
        OutputsPtr->actuator_withdraw[cup] = ActuatorWithdraw;
    }
    OutputsPtr->trigger_withdraw[cup] = TriggerWithdraw;
    if (TriggerBrake) {
        OutputsPtr->actuator_brake[cup] = ActuatorBrake;
    }
    OutputsPtr->trigger_brake[cup] = TriggerBrake;
    OutputsPtr->cup_error[cup] = Error;
}

// just for debug purposes, to see the state transitions in the console
static char *pneumaticFsmStateToString(PneumaticFsmStateEnum state) {
    switch (state) {
        case PNEUMATIC_FSM_STATE_BOOTED:
            return "BOOTED";
        case PNEUMATIC_FSM_STATE_EXTRACTED:
            return "EXTRACTED";
        case PNEUMATIC_FSM_STATE_INSERTING:
            return "INSERTING";
        case PNEUMATIC_FSM_STATE_INSERTED:
            return "INSERTED";
        case PNEUMATIC_FSM_STATE_WITHDRAWING:
            return "WITHDRAWING";
        case PNEUMATIC_FSM_STATE_ERROR:
            return "ERROR";
        case PNEUMATIC_FSM_STATE_UNDEFINED:
            return "UNDEFINED";
        default:
            return "UNKNOWN";
    }
}

static char *pneumaticWithLockFsmStateToString(PneumaticWithLockFsmStateEnum state) {
    switch (state) {
        case PNEUMATIC_WITH_LOCK_FSM_STATE_BOOTED:
            return "BOOTED";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED:
            return "EXTRACTED";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING:
            return "INSERTING";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED:
            return "INSERTED";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING:
            return "WITHDRAWING";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK:
            return "PAUSE_AFTER_LOCK";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTED:
            return "LOCKED_INSERTED";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK:
            return "PAUSE_AFTER_UNLOCK";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR:
            return "ERROR";
        case PNEUMATIC_WITH_LOCK_FSM_STATE_UNDEFINED:
            return "UNDEFINED";
        default:
            return "UNKNOWN";
    }
}

static char *motorFsmStateToString(MotorFsmStateEnum state) {
    switch (state) {
        case MOTOR_FSM_STATE_BOOTED:
            return "BOOTED";
        case MOTOR_FSM_STATE_EXTRACTED:
            return "EXTRACTED";
        case MOTOR_FSM_STATE_INSERTING:
            return "INSERTING";
        case MOTOR_FSM_STATE_INSERTED_PRE_BRAKING:
            return "INSERTED_PRE_BRAKING";
        case MOTOR_FSM_STATE_INSERTED_BRAKING:
            return "INSERTED_BRAKING";
        case MOTOR_FSM_STATE_INSERTED:
            return "INSERTED";
        case MOTOR_FSM_STATE_WITHDRAWING:
            return "WITHDRAWING";
        case MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING:
            return "WITHDRAWING_PRE_BRAKING";
        case MOTOR_FSM_STATE_WITHDRAWING_BRAKING:
            return "WITHDRAWING_BRAKING";
        case MOTOR_FSM_STATE_ERROR:
            return "ERROR";
        case MOTOR_FSM_STATE_UNDEFINED:
            return "UNDEFINED";
        default:
            return "UNKNOWN";
    }
}

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *inputs,
                       AuxiliaryFSMsState *FsmState,
                       AuxiliaryFSMsOutputs *outputs)
{
    // just for debug purposes, to see the state transitions in the console
    static AuxiliaryFSMsState DebugOldState = {0};
    static bool DebugOldStateInitialized = false;
    if (!DebugOldStateInitialized) {
        for (uint16_t cup = 0; cup < AUXILIARY_FSMS_MAX_CUPS; cup++) {
            DebugOldState.pneumatic_fsm_state[cup] = PNEUMATIC_FSM_STATE_UNDEFINED;
            DebugOldState.pneumatic_with_lock_fsm_state[cup] = PNEUMATIC_WITH_LOCK_FSM_STATE_UNDEFINED;
            DebugOldState.motor_fsm_state[cup] = MOTOR_FSM_STATE_UNDEFINED;
        }
        DebugOldStateInitialized = true;
    }

    if (!inputs || !FsmState || !outputs) {
        return;
    }

    memset(outputs, 0, sizeof(AuxiliaryFSMsOutputs));

    uint16_t installed_cups = clampInstalledCups(inputs->installed_cups);

    for (uint16_t Cup = 0; Cup < installed_cups; Cup++) {
        if (AUXILIARY_FSM_CUP_TYPE_PNEUMATIC == inputs->cup_type[Cup]) {
            pneumaticFsmTick(Cup, inputs, FsmState, outputs);

            // just for debug purposes, to see the state transitions in the console
            if (DebugOldState.pneumatic_fsm_state[Cup] != FsmState->pneumatic_fsm_state[Cup]) {
                printf("FSMs tick; Cup %u state %s -> %s\n", Cup, pneumaticFsmStateToString(DebugOldState.pneumatic_fsm_state[Cup]), 
                pneumaticFsmStateToString(FsmState->pneumatic_fsm_state[Cup]));
                DebugOldState.pneumatic_fsm_state[Cup] = FsmState->pneumatic_fsm_state[Cup];
            }
        } else if (AUXILIARY_FSM_CUP_TYPE_PNEUMATIC_WITH_LOCK == inputs->cup_type[Cup]) {
            pneumaticWithLockFsmTick(Cup, inputs, FsmState, outputs);

            // just for debug purposes, to see the state transitions in the console
            if (DebugOldState.pneumatic_with_lock_fsm_state[Cup] != FsmState->pneumatic_with_lock_fsm_state[Cup]) {
                printf("FSMs tick; Cup %u state %s -> %s\n", Cup, pneumaticWithLockFsmStateToString(DebugOldState.pneumatic_with_lock_fsm_state[Cup]), 
                pneumaticWithLockFsmStateToString(FsmState->pneumatic_with_lock_fsm_state[Cup]));
                DebugOldState.pneumatic_with_lock_fsm_state[Cup] = FsmState->pneumatic_with_lock_fsm_state[Cup];
            }
        } else if (AUXILIARY_FSM_CUP_TYPE_MOTOR == inputs->cup_type[Cup]) {
            motorFsmTick(Cup, inputs, FsmState, outputs);

            // just for debug purposes, to see the state transitions in the console
            if (DebugOldState.motor_fsm_state[Cup] != FsmState->motor_fsm_state[Cup]) {
                printf("FSMs tick; Cup %u state %s -> %s\n", Cup, motorFsmStateToString(DebugOldState.motor_fsm_state[Cup]), 
                motorFsmStateToString(FsmState->motor_fsm_state[Cup]));
                DebugOldState.motor_fsm_state[Cup] = FsmState->motor_fsm_state[Cup];
            }
        } else {
            printf("Error; file %s, line %d\n", __FILE__, __LINE__);
        }
    }
}
