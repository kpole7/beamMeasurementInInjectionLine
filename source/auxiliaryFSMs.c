/// @file auxiliaryFSMs.c
/// Auxiliary FSM logic for Faraday cup mechanisms.

#include "auxiliaryFSMs.h"
#include <limits.h>
#include <string.h>

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

static uint16_t saturatingAddU16(uint16_t a, uint16_t b)
{
    if ((uint32_t)a + (uint32_t)b > UINT16_MAX) {
        return UINT16_MAX;
    }
    return (uint16_t)(a + b);
}

static void evaluatePneumaticCup(bool requested,
                                 bool sw,
                                 bool *inserted,
                                 bool *steady,
                                 bool *transient,
                                 bool *actuator)
{
    *inserted = sw;
    *actuator = requested;

    if (requested) {
        *steady = sw;
    } else {
        *steady = !sw;
    }

    *transient = !(*steady);
}

static void evaluateMotorCup(bool requested,
                             bool sw1,
                             bool sw2,
                             bool *inserted,
                             bool *steady,
                             bool *transient,
                             bool *invalid,
                             bool *actuator)
{
    bool is_withdrawn = sw1 && !sw2;
    bool is_inserted = !sw1 && sw2;
    bool is_moving = !sw1 && !sw2;

    *invalid = sw1 && sw2;
    *inserted = is_inserted;
    *actuator = requested;

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
}

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *inputs,
                       AuxiliaryFSMsState *state,
                       AuxiliaryFSMsOutputs *outputs,
                       uint16_t tick_period_ms)
{
    if (!inputs || !state || !outputs) {
        return;
    }

    memset(outputs, 0, sizeof(AuxiliaryFSMsOutputs));

    uint16_t installed_cups = clampInstalledCups(inputs->installed_cups);

    for (uint16_t cup = 0; cup < installed_cups; cup++) {
        bool requested = inputs->cup_requested_state[cup];
        bool inserted = false;
        bool steady = false;
        bool transient = false;
        bool invalid = false;
        bool actuator = false;
        uint16_t error = 0u;

        if (inputs->cup_type[cup] == AUXILIARY_FSM_CUP_TYPE_PNEUMATIC) {
            evaluatePneumaticCup(requested,
                                inputs->cup_switch[cup],
                                &inserted,
                                &steady,
                                &transient,
                                &actuator);
        } else if (inputs->cup_type[cup] == AUXILIARY_FSM_CUP_TYPE_MOTOR) {
            evaluateMotorCup(requested,
                             inputs->cup_switch1[cup],
                             inputs->cup_switch2[cup],
                             &inserted,
                             &steady,
                             &transient,
                             &invalid,
                             &actuator);
        } else {
            error |= AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG;
            inserted = false;
            steady = false;
            transient = false;
            actuator = false;
        }

        if (invalid) {
            error |= AUXILIARY_FSM_ERROR_INVALID_SWITCHES;
        }

        if (transient) {
            uint16_t limit = requested ? inputs->time_limit_inserting_ms[cup]
                                       : inputs->time_limit_withdrawing_ms[cup];
            uint16_t effective_limit = (limit == 0u) ? 1u : limit;

            state->transition_elapsed_ms[cup] = saturatingAddU16(state->transition_elapsed_ms[cup], tick_period_ms);
            if (state->transition_elapsed_ms[cup] > effective_limit) {
                if (requested) {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_INSERT;
                } else {
                    error |= AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW;
                }
            }
        } else {
            state->transition_elapsed_ms[cup] = 0u;
        }

        outputs->actuator_control[cup] = actuator;
        outputs->cup_inserted[cup] = inserted;
        outputs->cup_steady[cup] = steady;
        outputs->cup_error[cup] = error;

        if (error != state->prev_error[cup]) {
            outputs->cup_last_error[cup] = state->prev_error[cup];
        }

        state->error_storage[cup] |= error;
        outputs->cup_error_storage[cup] = state->error_storage[cup];
        state->prev_error[cup] = error;
    }
}
