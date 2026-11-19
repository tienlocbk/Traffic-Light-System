/*
 * traffic_light.c - Traffic light driver for 2 roads
 * Each road uses 2 pins to control 3 LEDs (Green, Amber, Red, Off).
 */

#include "traffic_light.h"

/* -------------------------------------------------------------------------
 * Signal Combination Map (measured using TEST_STEP_3_2):
 *
 * Each module has 2 control pins (Signal A and Signal B).
 * 4 binary patterns map to 4 states:
 *
 *   State          Signal A    Signal B
 *   TRAFFIC_OFF       0           0
 *   TRAFFIC_GREEN     1           0
 *   TRAFFIC_AMBER     0           1
 *   TRAFFIC_RED       1           1
 *
 * If your hardware wiring or module variant uses a different mapping,
 * update the lookup table below accordingly.
 * ------------------------------------------------------------------------- */
typedef struct {
    GPIO_PinState pinA;
    GPIO_PinState pinB;
} TrafficPattern;

static const TrafficPattern signal_lut[4] = {
    [TRAFFIC_OFF]   = { GPIO_PIN_RESET, GPIO_PIN_RESET }, /* 0, 0 */
    [TRAFFIC_GREEN] = { GPIO_PIN_SET,   GPIO_PIN_RESET }, /* 1, 0 */
    [TRAFFIC_AMBER] = { GPIO_PIN_RESET, GPIO_PIN_SET   }, /* 0, 1 */
    [TRAFFIC_RED]   = { GPIO_PIN_SET,   GPIO_PIN_SET   }  /* 1, 1 */
};

/* Track current state of each road (1-indexed for convenience) */
static TrafficColor current_state[3] = { TRAFFIC_OFF, TRAFFIC_OFF, TRAFFIC_OFF };

static void write_pattern(GPIO_TypeDef *port, uint16_t pinA, uint16_t pinB,
                          TrafficPattern pattern)
{
    uint32_t set = 0U;
    uint32_t reset = 0U;

    if (pattern.pinA == GPIO_PIN_SET) set |= pinA;
    else reset |= pinA;
    if (pattern.pinB == GPIO_PIN_SET) set |= pinB;
    else reset |= pinB;

    /* Each road's two pins share GPIOB. One write avoids intermediate
     * colours when GREEN (10) changes to AMBER (01).
     */
    port->BSRR = set | (reset << 16U);
}

void traffic_init(void)
{
    traffic_all_off();
}

void traffic_set(uint8_t road, TrafficColor color)
{
    if ((unsigned)color > TRAFFIC_RED)
    {
        color = TRAFFIC_OFF;
    }

    TrafficPattern pat = signal_lut[color];

    if (road == ROAD_1)
    {
        write_pattern(TL1_A_GPIO_Port, TL1_A_Pin, TL1_B_Pin, pat);
        current_state[ROAD_1] = color;
    }
    else if (road == ROAD_2)
    {
        write_pattern(TL2_A_GPIO_Port, TL2_A_Pin, TL2_B_Pin, pat);
        current_state[ROAD_2] = color;
    }
}

TrafficColor traffic_get(uint8_t road)
{
    if (road == ROAD_1)
    {
        return current_state[ROAD_1];
    }
    if (road == ROAD_2)
    {
        return current_state[ROAD_2];
    }
    return TRAFFIC_OFF;
}

void traffic_toggle(uint8_t road, TrafficColor color)
{
    if (road != ROAD_1 && road != ROAD_2)
    {
        return;
    }

    if (current_state[road] == color)
    {
        traffic_set(road, TRAFFIC_OFF);
    }
    else
    {
        traffic_set(road, color);
    }
}

void traffic_all_off(void)
{
    traffic_set(ROAD_1, TRAFFIC_OFF);
    traffic_set(ROAD_2, TRAFFIC_OFF);
}

