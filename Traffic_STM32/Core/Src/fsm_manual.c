#include "fsm_manual.h"
#include "global.h"

void fsm_manual_enter(void)
{
    traffic.remaining_ms = 0U;
    traffic_phase_enter(RED1_GREEN2);
}

void fsm_manual_run(uint8_t events)
{
    if (traffic.mode != MODE_MANUAL || !(events & FSM_B2)) return;
    switch (traffic.phase) {
    case RED1_GREEN2: traffic_phase_enter(RED1_AMBER2); break;
    case RED1_AMBER2: traffic_phase_enter(GREEN1_RED2); break;
    case GREEN1_RED2: traffic_phase_enter(AMBER1_RED2); break;
    case AMBER1_RED2: traffic_phase_enter(RED1_GREEN2); break;
    default: fsm_manual_enter(); break;
    }
}
