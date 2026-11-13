#include "fsm_mode.h"
#include "global.h"
#include "fsm_automatic.h"
#include "fsm_manual.h"
#include "fsm_tuning.h"

static uint8_t button_events;

static void switch_safe(traffic_mode_t destination)
{
    traffic.mode = MODE_SWITCH_SAFE;
    traffic.pending_mode = destination;
    traffic.road1 = LIGHT_RED;
    traffic.road2 = LIGHT_RED;
    traffic.remaining_ms = FSM_CLEARANCE_MS;
    button_events = 0U;
}

void fsm_mode_init(void)
{
    traffic = (traffic_state_t){0};
    traffic.active = (traffic_timings_t){5U, 3U, 2U};
    traffic.draft = traffic.active;
    switch_safe(MODE_AUTO);
}

void fsm_mode_post_buttons(uint8_t events)
{
    /* A press during clearance must not become a press in the next mode. */
    if (traffic.mode != MODE_SWITCH_SAFE) button_events |= events & 0x0FU;
}

void fsm_mode_run(void)
{
    uint8_t events = button_events;
    button_events = 0U;

    /* Countdown is a software timer serviced by this periodic scheduler task. */
    if (traffic.remaining_ms > FSM_PERIOD_MS) traffic.remaining_ms -= FSM_PERIOD_MS;
    else traffic.remaining_ms = 0U;

    if (traffic.mode == MODE_SWITCH_SAFE) {
        if (traffic.remaining_ms != 0U) return;
        traffic.mode = traffic.pending_mode;
        switch (traffic.mode) {
        case MODE_AUTO: fsm_automatic_enter(); break;
        case MODE_MANUAL: fsm_manual_enter(); break;
        case MODE_TUNING: fsm_tuning_enter(); break;
        default: switch_safe(MODE_AUTO); break;
        }
        return;
    }

    /* B1 wins even when the phase timer expires in this same invocation. */
    if (events & FSM_B1) {
        switch (traffic.mode) {
        case MODE_AUTO: switch_safe(MODE_MANUAL); break;
        case MODE_MANUAL: switch_safe(MODE_TUNING); break;
        case MODE_TUNING:
            if (fsm_tuning_save()) switch_safe(MODE_AUTO);
            break;
        default: switch_safe(MODE_AUTO); break;
        }
        return;
    }

    switch (traffic.mode) {
    case MODE_AUTO: fsm_automatic_run(); break;
    case MODE_MANUAL: fsm_manual_run(events); break;
    case MODE_TUNING: fsm_tuning_run(events); break;
    default: switch_safe(MODE_AUTO); break;
    }
}
