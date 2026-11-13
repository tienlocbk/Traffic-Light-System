#include "fsm_automatic.h"
#include "global.h"

static void enter_phase(traffic_phase_t phase)
{
    traffic_phase_enter(phase);
    /* Reload replaces the expired timer, so no timeout survives a transition. */
    traffic.remaining_ms = 1000U *
        ((phase == RED1_GREEN2 || phase == GREEN1_RED2) ?
         traffic.active.green : traffic.active.amber);
}

void fsm_automatic_enter(void)
{
    enter_phase(RED1_GREEN2);
}

void fsm_automatic_run(void)
{
    if (traffic.mode != MODE_AUTO || traffic.remaining_ms != 0U) return;
    switch (traffic.phase) {
    case RED1_GREEN2: enter_phase(RED1_AMBER2); break;
    case RED1_AMBER2: enter_phase(GREEN1_RED2); break;
    case GREEN1_RED2: enter_phase(AMBER1_RED2); break;
    case AMBER1_RED2: enter_phase(RED1_GREEN2); break;
    default: fsm_automatic_enter(); break;
    }
}
