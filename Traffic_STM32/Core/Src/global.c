#include "global.h"

traffic_state_t traffic;

void traffic_phase_enter(traffic_phase_t phase)
{
    traffic.phase = phase;
    switch (phase) {
    case RED1_GREEN2:
        traffic.road1 = LIGHT_RED;
        traffic.road2 = LIGHT_GREEN;
        break;
    case RED1_AMBER2:
        traffic.road1 = LIGHT_RED;
        traffic.road2 = LIGHT_AMBER;
        break;
    case GREEN1_RED2:
        traffic.road2 = LIGHT_RED;
        traffic.road1 = LIGHT_GREEN;
        break;
    case AMBER1_RED2:
        traffic.road1 = LIGHT_AMBER;
        traffic.road2 = LIGHT_RED;
        break;
    default:
        traffic.road1 = LIGHT_RED;
        traffic.road2 = LIGHT_RED;
        break;
    }
}

uint8_t traffic_timings_valid(const traffic_timings_t *t)
{
    return t->red >= 1U && t->red <= 99U &&
           t->green >= 1U && t->green <= 99U &&
           t->amber >= 1U && t->amber <= 99U &&
           t->red == (uint16_t)t->green + t->amber;
}

uint8_t traffic_countdown(uint8_t road)
{
    uint32_t seconds;
    if (traffic.mode != MODE_AUTO || (road != 1U && road != 2U)) return 0U;
    seconds = (traffic.remaining_ms + 999U) / 1000U;
    if ((road == 1U && traffic.phase == RED1_GREEN2) ||
        (road == 2U && traffic.phase == GREEN1_RED2)) {
        seconds += traffic.active.amber;
    }
    return (uint8_t)seconds;
}
