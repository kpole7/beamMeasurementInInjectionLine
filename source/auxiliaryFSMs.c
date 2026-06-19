/// @file auxiliaryFSMs.c
/// Auxiliary FSM logic for Faraday cup mechanisms.

#include "auxiliaryFSMs.h"
#include "debuggingTools.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>


#define PAUSE_AFTER_BOOT_TIME_IN_TICKS 500u // 1 second with 2ms tick period
#define PRE_BRAKING_TIME_IN_TICKS 5u
#define BRAKING_TIME_IN_TICKS 400u
#define PAUSE_AFTER_LOCK_TIME_IN_TICKS 500u
#define PAUSE_AFTER_UNLOCK_TIME_IN_TICKS 250u

static uint16_t clampInstalledCups(uint16_t Value)
{
    if (Value < 1u) {
        return 1u;
    }
    if (Value > MAX_CUPS) {
        return MAX_CUPS;
    }
    return Value;
}

static inline void saturatingIncreaseU16(uint16_t *A) {
    if (*A < UINT16_MAX) {
        (*A)++;
    }
}

// -------------------------------------------------------------------------------------------------------------
// FSM implementation for simple pneumatic actuator
// -------------------------------------------------------------------------------------------------------------

static void pneumaticFsmBooted(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (!PauseAfterBootFinished) {
        return;
    }
    if (!Switch) {
        *StatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    *StatePtr = PNEUMATIC_FSM_STATE_INSERTED;
}

static void pneumaticFsmExtracted(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (Switch) {
        *StatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    if (Requested) {
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_FSM_STATE_INSERTING;
#if 0
        printf("New state=%d; requested=%d, sw=%d; file %s, line %d\n", *StatePtr, Requested, Switch, __FILE__, __LINE__);
#endif
    }
}

static void pneumaticFsmInserting(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (Switch) {
        *StatePtr = PNEUMATIC_FSM_STATE_INSERTED;
    }
    if (TransitionTimeFinished) {
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
    }
}

static void pneumaticFsmInserted(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (!Switch) {
        *StatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    if (!Requested) {
        *ActuatorPtr = false;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_FSM_STATE_WITHDRAWING;
    }
}

static void pneumaticFsmWithdrawing(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (!Switch) {
        *StatePtr = PNEUMATIC_FSM_STATE_EXTRACTED;
    }
    if (TransitionTimeFinished) {
        *ActuatorPtr = false;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
    }
}

static void pneumaticFsmOnError(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (Recovery) {
        *ErrorPtr = 0u;
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_FSM_STATE_INSERTING;
        printf("Attempt to recover; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
    }
}

static void evaluatePneumaticCup(bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool TransitionTimeFinished, 
    PneumaticFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr, bool *IsCupInsertedPtr) 
    {
    switch (*StatePtr) {
        case PNEUMATIC_FSM_STATE_BOOTED:
            pneumaticFsmBooted(Requested, Recovery, Switch, PauseAfterBootFinished, TransitionTimeFinished, StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_EXTRACTED:
            *IsCupInsertedPtr = false;
            pneumaticFsmExtracted(Requested, Recovery, Switch, PauseAfterBootFinished, TransitionTimeFinished, StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_INSERTING:
            pneumaticFsmInserting(Requested, Recovery, Switch, PauseAfterBootFinished, TransitionTimeFinished, StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_INSERTED:
            *IsCupInsertedPtr = true;
            pneumaticFsmInserted(Requested, Recovery, Switch, PauseAfterBootFinished, TransitionTimeFinished, StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_WITHDRAWING:
            pneumaticFsmWithdrawing(Requested, Recovery, Switch, PauseAfterBootFinished, TransitionTimeFinished, StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_FSM_STATE_ERROR:
            pneumaticFsmOnError(Requested, Recovery, Switch, PauseAfterBootFinished, TransitionTimeFinished, StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        default:
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;
}

void pneumaticFsmTick(uint16_t Cup, 
                      const AuxiliaryFSMsInputs *InputsPtr,
                      AuxiliaryFSMsState *FsmStatePtr,
                      AuxiliaryFSMsOutputs *OutputsPtr)
{
    PneumaticFsmStateEnum PneumaticLocalState = FsmStatePtr->pneumatic_fsm_state[Cup];
    bool PauseAfterBootIsOver = false;
    bool TransitionTimeLimitExceeded = false;
    bool Actuator = false;
    bool Trigger = false;
    uint16_t Error = 0u;

    if (InputsPtr->cup_type[Cup] != CUP_TYPE_PNEUMATIC){
        return;
    }

    if (PNEUMATIC_FSM_STATE_BOOTED == PneumaticLocalState) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_boot_elapsed[Cup]);
        if (FsmStatePtr->pause_after_boot_elapsed[Cup] > PAUSE_AFTER_BOOT_TIME_IN_TICKS) {
            PauseAfterBootIsOver = true;
        }
    } else {
        FsmStatePtr->pause_after_boot_elapsed[Cup] = 0u;
    }

    evaluatePneumaticCup(InputsPtr->cup_requested_state[Cup],
                         InputsPtr->cup_error_recover[Cup],
                         InputsPtr->cup_switch[Cup],
                         PauseAfterBootIsOver,
                         TransitionTimeLimitExceeded,
                         &PneumaticLocalState,
                         &Actuator,
                         &Trigger,
                         &Error,
                         &FsmStatePtr->is_cup_inserted[Cup]);
    if (PneumaticLocalState == PNEUMATIC_FSM_STATE_INSERTING){
        uint16_t effective_limit = (InputsPtr->time_limit_inserting_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_inserting_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            TransitionTimeLimitExceeded = true;
        }
    } else if (PneumaticLocalState == PNEUMATIC_FSM_STATE_WITHDRAWING) {
        uint16_t effective_limit = (InputsPtr->time_limit_withdrawing_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_withdrawing_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            TransitionTimeLimitExceeded = true;
        }
    } else {
        TransitionTimeLimitExceeded = false;
        FsmStatePtr->transition_elapsed[Cup] = 0u;
    }

    FsmStatePtr->pneumatic_fsm_state[Cup] = PneumaticLocalState;
    if (Trigger) {
        OutputsPtr->actuator_insert[Cup] = Actuator;
    }
    OutputsPtr->trigger_insert[Cup] = Trigger;
    OutputsPtr->cup_error[Cup] = Error;
}

// -------------------------------------------------------------------------------------------------------------
// FSM implementation for pneumatic actuator with lock
// -------------------------------------------------------------------------------------------------------------

static void pneumaticWithLockFsmBooted(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
{

    if (Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK;
    }
    else{
        if (!PauseAfterBootFinished) {
            // wait until the pause after boot is over
            return;
        }

        if (!Switch) {
            *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR;
            *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC_WITH_LOCK;
            printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
            return;
        }
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED;
    }
}

static void pneumaticWithLockFsmInserted(bool Inhibit, bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool PauseAfterLockFinished,
    bool PauseAfterUnlockFinished, PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr,
    bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK;
        return;
    }
    if (!Switch) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC_WITH_LOCK;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    if (!Requested) {
        *ActuatorPtr = false;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING;
    }
}

static void pneumaticWithLockFsmWithdrawing(bool Inhibit, bool Requested, bool Recovery, bool Switch, bool PauseAfterBootFinished, bool PauseAfterLockFinished,
    bool PauseAfterUnlockFinished, PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr,
    bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK;
        return;
    }
    if (!Switch) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED;
    }
}

static void pneumaticWithLockFsmExtracted(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK;
        return;
    }
    if (Switch) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC_WITH_LOCK;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    if (Requested) {
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING;
    }
}

static void pneumaticWithLockFsmInserting(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK;
        return;
    }
    if (Switch) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED;
    }
}

static void pneumaticWithLockFsmPauseAfterLock(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
    {
    if (!Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK;
        return;
    }
    if (!PauseAfterLockFinished) {
        // wait until the pause after lock is over
        return;
    }
    // the pause after lockdown is over, the cup should be inserted
    if (!Switch) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC_WITH_LOCK;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
    *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED;
}

static void pneumaticWithLockFsmLockedInserted(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (!Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK;
        return;
    }
    if (!Switch) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC_WITH_LOCK;
        printf("Error; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
        return;
    }
}

static void pneumaticWithLockFsmPauseAfterUnlock(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (Inhibit) {
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK;
        return;
    }
    if (!PauseAfterUnlockFinished) {
        // wait until the pause after unlock is over
        return;
    }
    // the pause after unlock is over, recover the cup to the requested state
    if (Requested) {
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING;
    } else {
        *ActuatorPtr = false;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING;
    }
}

static void pneumaticWithLockFsmOnError(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr) 
{
    if (Recovery) {
        if (Inhibit) {
            printf("Recovery is not posible during inhibition\n" );
            return;
        }
        *ErrorPtr = 0u;
        *ActuatorPtr = true;
        *TriggerPtr = true;
        *StatePtr = PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING;
        printf("Attempt to recover; requested=%d, sw=%d; file %s, line %d\n", Requested, Switch, __FILE__, __LINE__);
    }
}

static void evaluatePneumaticWithLockCup(bool Inhibit, bool Requested, bool Recovery, bool Switch, 
    bool PauseAfterBootFinished, bool PauseAfterLockFinished, bool PauseAfterUnlockFinished,
    PneumaticWithLockFsmStateEnum *StatePtr, bool *ActuatorPtr, bool *TriggerPtr, uint16_t *ErrorPtr, bool *IsCupInsertedPtr) 
    {
    switch (*StatePtr) {
        case PNEUMATIC_WITH_LOCK_FSM_STATE_BOOTED:
            pneumaticWithLockFsmBooted(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED:
            *IsCupInsertedPtr = true;
            pneumaticWithLockFsmInserted(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING:
            pneumaticWithLockFsmWithdrawing(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED:
            *IsCupInsertedPtr = false;
            pneumaticWithLockFsmExtracted(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING:
            pneumaticWithLockFsmInserting(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK:
            pneumaticWithLockFsmPauseAfterLock(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTED:
            *IsCupInsertedPtr = true;
            pneumaticWithLockFsmLockedInserted(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK:
            pneumaticWithLockFsmPauseAfterUnlock(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        case PNEUMATIC_WITH_LOCK_FSM_STATE_ERROR:
            pneumaticWithLockFsmOnError(Inhibit, Requested, Recovery, Switch, PauseAfterBootFinished, PauseAfterLockFinished, PauseAfterUnlockFinished, 
                StatePtr, ActuatorPtr, TriggerPtr, ErrorPtr);
            break;
        default:
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;
}

void pneumaticWithLockFsmTick(uint16_t Cup, 
                              const AuxiliaryFSMsInputs *InputsPtr,
                              AuxiliaryFSMsState *FsmStatePtr,
                              AuxiliaryFSMsOutputs *OutputsPtr)
{
    bool Inhibit = InputsPtr->external_inhibition;
    PneumaticWithLockFsmStateEnum PneumaticWithLockLocalState = FsmStatePtr->pneumatic_with_lock_fsm_state[Cup];
    bool Actuator = false;
    bool Trigger = false;
    bool PauseAfterBootIsOver = false;
    bool PauseAfterLockIsOver = false;
    bool PauseAfterUnlockIsOver = false;
    uint16_t Error = 0u;

    if (InputsPtr->cup_type[Cup] != CUP_TYPE_PNEUMATIC_WITH_LOCK) {
        return;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_BOOTED == PneumaticWithLockLocalState) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_boot_elapsed[Cup]);
        if (FsmStatePtr->pause_after_boot_elapsed[Cup] > PAUSE_AFTER_BOOT_TIME_IN_TICKS) {
            PauseAfterBootIsOver = true;
        }
    } else {
        FsmStatePtr->pause_after_boot_elapsed[Cup] = 0u;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK == PneumaticWithLockLocalState) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_lock_elapsed[Cup]);
        if (FsmStatePtr->pause_after_lock_elapsed[Cup] > PAUSE_AFTER_LOCK_TIME_IN_TICKS) {
            PauseAfterLockIsOver = true;
        }
    } else {
        FsmStatePtr->pause_after_lock_elapsed[Cup] = 0u;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK == PneumaticWithLockLocalState) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_unlock_elapsed[Cup]);
        if (FsmStatePtr->pause_after_unlock_elapsed[Cup] > PAUSE_AFTER_UNLOCK_TIME_IN_TICKS) {
            PauseAfterUnlockIsOver = true;
        }
    } else {
        FsmStatePtr->pause_after_unlock_elapsed[Cup] = 0u;
    }
    if (PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING == PneumaticWithLockLocalState){
        uint16_t effective_limit = (InputsPtr->time_limit_inserting_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_inserting_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
        }
    } else if (PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING == PneumaticWithLockLocalState) {
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
                                 InputsPtr->cup_error_recover[Cup],
                                 InputsPtr->cup_switch[Cup],
                                 PauseAfterBootIsOver,
                                 PauseAfterLockIsOver,
                                 PauseAfterUnlockIsOver,
                                 &PneumaticWithLockLocalState,
                                 &Actuator,
                                 &Trigger,
                                 &Error,
                                 &FsmStatePtr->is_cup_inserted[Cup]);
    FsmStatePtr->pneumatic_with_lock_fsm_state[Cup] = PneumaticWithLockLocalState;
    if (Trigger) {
        OutputsPtr->actuator_insert[Cup] = Actuator;
    }
    OutputsPtr->trigger_insert[Cup] = Trigger;
    OutputsPtr->cup_error[Cup] = Error;
}

// -------------------------------------------------------------------------------------------------------------
// FSM implementation for motor actuator
// -------------------------------------------------------------------------------------------------------------

static void motorFsmBooted(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (!PauseAfterBootFinished) {
        return;
    }
    if (SwitchA && SwitchB) {
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCHES_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (SwitchA && !SwitchB) {
        *StatePtr = MOTOR_FSM_STATE_INSERTED;
        return;
    }
    if (!SwitchA && SwitchB) {
        *StatePtr = MOTOR_FSM_STATE_EXTRACTED;
        return;
    }
    // If both switches are off, we assume the cup is in the middle position
    // and we will move it to the inserted position by default.
    *ActuatorInsertPtr = true;
    *TriggerInsertPtr = true;
    *StatePtr = MOTOR_FSM_STATE_INSERTING;
    printf("Warning! Initial restoration of the inserted position; requested=%d, swA=%d, swB=%d;\n  file %s, line %d\n", 
        Requested, SwitchA, SwitchB, __FILE__, __LINE__);
}

static void motorFsmInserted(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (SwitchA && SwitchB) {
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCHES_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (!SwitchA) {
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_A_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (!Requested) {
        *ActuatorWithdrawPtr = true;
        *TriggerWithdrawPtr = true;
        *StatePtr = MOTOR_FSM_STATE_WITHDRAWING;
    }
}

static void motorFsmWithdrawing(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (SwitchA && SwitchB) {
        *ActuatorWithdrawPtr = false;
        *TriggerWithdrawPtr = true;
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCHES_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (SwitchB) {
        *ActuatorWithdrawPtr = false;
        *TriggerWithdrawPtr = true;
        *StatePtr = MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING;
    }
}

static void motorFsmWithdrawingPreBraking(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (!IsPreBrakingComplete) {
        // wait until the pre-braking is complete
        return;
    }
    *ActuatorBrakePtr = true;
    *TriggerBrakePtr = true;
    *StatePtr = MOTOR_FSM_STATE_WITHDRAWING_BRAKING;
}

static void motorFsmWithdrawingBraking(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (!IsBrakingComplete) {
        // wait until the braking is complete
        return;
    }
    *ActuatorBrakePtr = false;
    *TriggerBrakePtr = true;
    *StatePtr = MOTOR_FSM_STATE_EXTRACTED;
}

static void motorFsmExtracted(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (SwitchA && SwitchB) {
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCHES_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (!SwitchB) {
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCH_B_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (Requested) {
        *ActuatorInsertPtr = true;
        *TriggerInsertPtr = true;
        *StatePtr = MOTOR_FSM_STATE_INSERTING;
    }
}

static void motorFsmInserting(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (SwitchA && SwitchB) {
        *ActuatorInsertPtr = false;
        *TriggerInsertPtr = true;
        *StatePtr = MOTOR_FSM_STATE_ERROR;
        *ErrorPtr |= AUXILIARY_FSM_ERROR_SWITCHES_OF_MOTOR_ACTUATOR;
        printf("Error; requested=%d, swA=%d, swB=%d; file %s, line %d\n", Requested, SwitchA, SwitchB, __FILE__, __LINE__);
        return;
    }
    if (SwitchA) {
        *ActuatorInsertPtr = false;
        *TriggerInsertPtr = true;
        *StatePtr = MOTOR_FSM_STATE_INSERTING_PRE_BRAKING;
    }
}

static void motorFsmInsertingPreBraking(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (!IsPreBrakingComplete) {
        // wait until the pre-braking is complete
        return;
    }
    *ActuatorBrakePtr = true;
    *TriggerBrakePtr = true;
    *StatePtr = MOTOR_FSM_STATE_INSERTING_BRAKING;
}

static void motorFsmInsertingBraking(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    if (!IsBrakingComplete) {
        // wait until the braking is complete
        return;
    }
    *ActuatorBrakePtr = false;
    *TriggerBrakePtr = true;
    *StatePtr = MOTOR_FSM_STATE_INSERTED;
}

static void motorFsmOnError(bool Requested, bool Recovery, bool SwitchA, bool SwitchB, bool PauseAfterBootFinished, bool IsPreBrakingComplete, bool IsBrakingComplete, 
    MotorFsmStateEnum *StatePtr, bool *ActuatorInsertPtr, bool *ActuatorWithdrawPtr, bool *ActuatorBrakePtr, 
    bool *TriggerInsertPtr, bool *TriggerWithdrawPtr, bool *TriggerBrakePtr, uint16_t *ErrorPtr)
{
    *ActuatorInsertPtr = false;
    *TriggerInsertPtr = true;
    *ActuatorWithdrawPtr = false;
    *TriggerWithdrawPtr = true;
    *ActuatorBrakePtr = false;
    *TriggerBrakePtr = true;
    *StatePtr = MOTOR_FSM_STATE_BOOTED;
}

static void evaluateMotorCup(bool Requested,
                             bool Recovery,
                             bool SwitchA,
                             bool SwitchB,
                             bool IsPauseAfterBootFinished,
                             bool IsPreBrakingComplete,
                             bool IsBrakingComplete,
                             MotorFsmStateEnum *StatePtr,
                             bool *ActuatorInsertPtr,
                             bool *ActuatorWithdrawPtr,
                             bool *ActuatorBrakePtr,
                             bool *TriggerInsertPtr,
                             bool *TriggerWithdrawPtr,
                             bool *TriggerBrakePtr,
                             uint16_t *ErrorPtr,
                             bool *IsCupInsertedPtr)
{
    switch (*StatePtr) {
        case MOTOR_FSM_STATE_BOOTED:
            motorFsmBooted(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTED:
            *IsCupInsertedPtr = true;
            motorFsmInserted(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING:
            motorFsmWithdrawing(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING:
            motorFsmWithdrawingPreBraking(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_WITHDRAWING_BRAKING:
            motorFsmWithdrawingBraking(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_EXTRACTED:
            *IsCupInsertedPtr = false;
            motorFsmExtracted(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTING:
            motorFsmInserting(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTING_PRE_BRAKING:
            motorFsmInsertingPreBraking(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_INSERTING_BRAKING:
            motorFsmInsertingBraking(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        case MOTOR_FSM_STATE_ERROR:
            motorFsmOnError(Requested, Recovery, SwitchA, SwitchB, IsPauseAfterBootFinished, IsPreBrakingComplete, IsBrakingComplete, StatePtr, ActuatorInsertPtr, 
                ActuatorWithdrawPtr, ActuatorBrakePtr, TriggerInsertPtr, TriggerWithdrawPtr, TriggerBrakePtr, ErrorPtr);
            break;
        default:
            *ErrorPtr |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            break;
    }
    return;
}

void motorFsmTick(uint16_t Cup, 
                  const AuxiliaryFSMsInputs *InputsPtr,
                  AuxiliaryFSMsState *FsmStatePtr,
                  AuxiliaryFSMsOutputs *OutputsPtr)
{
    MotorFsmStateEnum MotorLocalState = FsmStatePtr->motor_fsm_state[Cup];
    bool PauseAfterBootIsOver = false;
    bool PreBrakingTimeExceeded = false;
    bool BrakingTimeExceeded = false;
    bool ActuatorInsert = false;
    bool ActuatorWithdraw = false;
    bool ActuatorBrake = false;
    bool TriggerInsert = false;
    bool TriggerWithdraw = false;
    bool TriggerBrake = false;
    uint16_t Error = 0u;

    if (InputsPtr->cup_type[Cup] != CUP_TYPE_MOTOR) {
        return;
    }

    if (MOTOR_FSM_STATE_BOOTED == MotorLocalState) {
        saturatingIncreaseU16(&FsmStatePtr->pause_after_boot_elapsed[Cup]);
        if (FsmStatePtr->pause_after_boot_elapsed[Cup] > PAUSE_AFTER_BOOT_TIME_IN_TICKS) {
            PauseAfterBootIsOver = true;
        }
    } else {
        FsmStatePtr->pause_after_boot_elapsed[Cup] = 0u;
    }
    if ((MOTOR_FSM_STATE_INSERTING_PRE_BRAKING == MotorLocalState) || (MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING == MotorLocalState)){
        saturatingIncreaseU16(&FsmStatePtr->pre_braking_elapsed[Cup]);
        if (FsmStatePtr->pre_braking_elapsed[Cup] > PRE_BRAKING_TIME_IN_TICKS) {
            PreBrakingTimeExceeded = true;
        }
    } else {
        FsmStatePtr->pre_braking_elapsed[Cup] = 0u;
    }
    if ((MOTOR_FSM_STATE_INSERTING_BRAKING == MotorLocalState) || (MOTOR_FSM_STATE_WITHDRAWING_BRAKING == MotorLocalState)){
        saturatingIncreaseU16(&FsmStatePtr->braking_elapsed[Cup]);
        if (FsmStatePtr->braking_elapsed[Cup] > BRAKING_TIME_IN_TICKS) {
            BrakingTimeExceeded = true;
        }
    } else {
        FsmStatePtr->braking_elapsed[Cup] = 0u;
    }
    if (MOTOR_FSM_STATE_INSERTING == MotorLocalState){
        uint16_t effective_limit = (InputsPtr->time_limit_inserting_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_inserting_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
        }
    } else if (MOTOR_FSM_STATE_WITHDRAWING == MotorLocalState) {
        uint16_t effective_limit = (InputsPtr->time_limit_withdrawing_ms[Cup] == 0u) ? 1u : InputsPtr->time_limit_withdrawing_ms[Cup];
        saturatingIncreaseU16(&FsmStatePtr->transition_elapsed[Cup]);
        if (FsmStatePtr->transition_elapsed[Cup] > effective_limit) {
            Error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
        }
    } else {
        FsmStatePtr->transition_elapsed[Cup] = 0u;
    }
    evaluateMotorCup(InputsPtr->cup_requested_state[Cup],
                     InputsPtr->cup_error_recover[Cup],
                     InputsPtr->cup_switch_a[Cup],
                     InputsPtr->cup_switch_b[Cup],
                     PauseAfterBootIsOver,
                     PreBrakingTimeExceeded,
                     BrakingTimeExceeded,
                     &MotorLocalState,
                     &ActuatorInsert,
                     &ActuatorWithdraw,
                     &ActuatorBrake,
                     &TriggerInsert,
                     &TriggerWithdraw,
                     &TriggerBrake,
                     &Error,
                     &FsmStatePtr->is_cup_inserted[Cup]);

    FsmStatePtr->motor_fsm_state[Cup] = MotorLocalState;
    if (TriggerInsert) {
        OutputsPtr->actuator_insert[Cup] = ActuatorInsert;
    }
    OutputsPtr->trigger_insert[Cup] = TriggerInsert;
    if (TriggerWithdraw) {
        OutputsPtr->actuator_withdraw[Cup] = ActuatorWithdraw;
    }
    OutputsPtr->trigger_withdraw[Cup] = TriggerWithdraw;
    if (TriggerBrake) {
        OutputsPtr->actuator_brake[Cup] = ActuatorBrake;
    }
    OutputsPtr->trigger_brake[Cup] = TriggerBrake;
    OutputsPtr->cup_error[Cup] = Error;
}

// -------------------------------------------------------------------------------------------------------------
// The module interface functions
// -------------------------------------------------------------------------------------------------------------

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
        case MOTOR_FSM_STATE_INSERTED:
            return "INSERTED";
        case MOTOR_FSM_STATE_WITHDRAWING:
            return "WITHDRAWING";
        case MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING:
            return "WITHDRAWING_PRE_BRAKING";
        case MOTOR_FSM_STATE_WITHDRAWING_BRAKING:
            return "WITHDRAWING_BRAKING";
        case MOTOR_FSM_STATE_EXTRACTED:
            return "EXTRACTED";
        case MOTOR_FSM_STATE_INSERTING:
            return "INSERTING";
        case MOTOR_FSM_STATE_INSERTING_PRE_BRAKING:
            return "INSERTING_PRE_BRAKING";
        case MOTOR_FSM_STATE_INSERTING_BRAKING:
            return "INSERTING_BRAKING";
        case MOTOR_FSM_STATE_ERROR:
            return "ERROR";
        case MOTOR_FSM_STATE_UNDEFINED:
            return "UNDEFINED";
        default:
            return "UNKNOWN";
    }
}

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *Inputs,
                       AuxiliaryFSMsState *FsmState,
                       AuxiliaryFSMsOutputs *Outputs)
{
    // just for debug purposes, to see the state transitions in the console
    static AuxiliaryFSMsState DebugOldState = {0};
    static bool DebugOldStateInitialized = false;
    if (!DebugOldStateInitialized) {
        for (uint16_t cup = 0; cup < MAX_CUPS; cup++) {
            DebugOldState.pneumatic_fsm_state[cup] = PNEUMATIC_FSM_STATE_UNDEFINED;
            DebugOldState.pneumatic_with_lock_fsm_state[cup] = PNEUMATIC_WITH_LOCK_FSM_STATE_UNDEFINED;
            DebugOldState.motor_fsm_state[cup] = MOTOR_FSM_STATE_UNDEFINED;
        }
        DebugOldStateInitialized = true;
    }

    if (!Inputs || !FsmState || !Outputs) {
        return;
    }

    memset(Outputs, 0, sizeof(AuxiliaryFSMsOutputs));

    uint16_t installed_cups = clampInstalledCups(Inputs->installed_cups);

    for (uint16_t Cup = 0; Cup < installed_cups; Cup++) {
        if (CUP_TYPE_PNEUMATIC == Inputs->cup_type[Cup]) {
            pneumaticFsmTick(Cup, Inputs, FsmState, Outputs);

            // just for debug purposes, to see the state transitions in the console
            if (DebugOldState.pneumatic_fsm_state[Cup] != FsmState->pneumatic_fsm_state[Cup]) {
                printf("%s  FSMs tick; Cup %u state %s -> %s\n", getTimeStampString(), Cup, pneumaticFsmStateToString(DebugOldState.pneumatic_fsm_state[Cup]), 
                pneumaticFsmStateToString(FsmState->pneumatic_fsm_state[Cup]));
                DebugOldState.pneumatic_fsm_state[Cup] = FsmState->pneumatic_fsm_state[Cup];
            }
        } else if (CUP_TYPE_PNEUMATIC_WITH_LOCK == Inputs->cup_type[Cup]) {
            pneumaticWithLockFsmTick(Cup, Inputs, FsmState, Outputs);

            // just for debug purposes, to see the state transitions in the console
            if (DebugOldState.pneumatic_with_lock_fsm_state[Cup] != FsmState->pneumatic_with_lock_fsm_state[Cup]) {
                printf("%s  FSMs tick; Cup %u state %s -> %s\n", getTimeStampString(), Cup, pneumaticWithLockFsmStateToString(DebugOldState.pneumatic_with_lock_fsm_state[Cup]), 
                pneumaticWithLockFsmStateToString(FsmState->pneumatic_with_lock_fsm_state[Cup]));
                DebugOldState.pneumatic_with_lock_fsm_state[Cup] = FsmState->pneumatic_with_lock_fsm_state[Cup];
            }
        } else if (CUP_TYPE_MOTOR == Inputs->cup_type[Cup]) {
            motorFsmTick(Cup, Inputs, FsmState, Outputs);

            // just for debug purposes, to see the state transitions in the console
            if (DebugOldState.motor_fsm_state[Cup] != FsmState->motor_fsm_state[Cup]) {
                printf("%s  FSMs tick; Cup %u state %s -> %s\n", getTimeStampString(), Cup, motorFsmStateToString(DebugOldState.motor_fsm_state[Cup]), 
                motorFsmStateToString(FsmState->motor_fsm_state[Cup]));
                DebugOldState.motor_fsm_state[Cup] = FsmState->motor_fsm_state[Cup];
            }
        } else {
            printf("Error; file %s, line %d\n", __FILE__, __LINE__);
        }
        if (FsmState->is_cup_inserted[Cup]) {
            FsmState->active_cup = Cup+1; // we use 1-based indexing for active cup
        }
    }
    if (DebugOldState.active_cup != FsmState->active_cup) {
        printf("%s  FSMs tick; active cup %u -> %u\n", getTimeStampString(), DebugOldState.active_cup, FsmState->active_cup);
        DebugOldState.active_cup = FsmState->active_cup;
    }
}
