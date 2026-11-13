#include "fsm_tuning.h"
#include "global.h"

void fsm_tuning_enter(void)
{
    traffic.selection = SET_RED;
    traffic.draft = traffic.active;
    traffic.validation_error = 0U;
    traffic.remaining_ms = 0U;
    traffic.road1 = LIGHT_RED;
    traffic.road2 = LIGHT_RED;
}

uint8_t fsm_tuning_save(void)
{
    if (traffic.mode != MODE_TUNING) return 0U;
    if (!traffic_timings_valid(&traffic.draft)) {
        traffic.validation_error = 1U;
        return 0U;
    }
    traffic.active = traffic.draft;
    traffic.validation_error = 0U;
    return 1U;
}

void fsm_tuning_run(uint8_t events)
{
    uint8_t *value;
    if (traffic.mode != MODE_TUNING) return;
    if (events & FSM_B2) {
        traffic.selection = (tuning_state_t)((traffic.selection + 1U) % 3U);
        return;
    }
    switch (traffic.selection) {
    case SET_RED: value = &traffic.draft.red; break;
    case SET_GREEN: value = &traffic.draft.green; break;
    case SET_AMBER: value = &traffic.draft.amber; break;
    default: return;
    }
    if (events & FSM_B3) {
        if (*value < 99U) ++*value;
        traffic.validation_error = 0U;
    } else if (events & FSM_B4) {
        if (*value > 1U) --*value;
        traffic.validation_error = 0U;
    }
}
