#include "fsm_tuning.h"
#include "global.h"

void fsm_tuning_enter(void)
{
    traffic.selection = SET_GREEN;
    traffic.draft = traffic.active;
    traffic.draft.red = traffic.draft.green + traffic.draft.amber;
    traffic.remaining_ms = 0U;
    traffic.road1 = LIGHT_RED;
    traffic.road2 = LIGHT_RED;
}

uint8_t fsm_tuning_save(void)
{
    if (traffic.mode != MODE_TUNING) return 0U;
    /* Editing preserves valid G/A bounds, so every user draft can be saved. */
    traffic.draft.red = traffic.draft.green + traffic.draft.amber;
    traffic.active = traffic.draft;
    return 1U;
}

void fsm_tuning_run(uint8_t events)
{
    uint8_t *value;
    if (traffic.mode != MODE_TUNING) return;
    if (events & FSM_B2) {
        traffic.selection = (traffic.selection == SET_GREEN) ? SET_AMBER : SET_GREEN;
        return;
    }
    switch (traffic.selection) {
    case SET_GREEN: value = &traffic.draft.green; break;
    case SET_AMBER: value = &traffic.draft.amber; break;
    default: return;
    }
    if (events & FSM_B3) {
        if ((uint16_t)traffic.draft.green + traffic.draft.amber < 99U) ++*value;
    } else if (events & FSM_B4) {
        if (*value > 1U) --*value;
    }
    traffic.draft.red = traffic.draft.green + traffic.draft.amber;
}
