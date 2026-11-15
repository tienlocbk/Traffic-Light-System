/*
 * software_timer.c - Software timer implementation
 * Decrements counters every 10ms (1 tick of scheduler)
 */

#include "software_timer.h"

/* Array of timer expired flags (publicly accessible as extern int timer_flag[]) */
int timer_flag[TIMER_MAX] = {0};

/* Array of remaining tick counters for each timer (internal) */
static uint32_t timer_counter[TIMER_MAX] = {0};

void software_timer_init(void)
{
    for (uint8_t i = 0; i < TIMER_MAX; i++)
    {
        timer_counter[i] = 0;
        timer_flag[i] = 0;
    }
}

void setTimer(uint8_t index, uint32_t duration_ms)
{
    if (index >= TIMER_MAX)
    {
        return;
    }

    if (duration_ms == 0)
    {
        timer_counter[index] = 0;
        timer_flag[index] = 1;
        return;
    }

    /* Convert milliseconds to ticks (each tick is TIMER_TICK_MS = 10 ms).
     * Ceiling division ensures short non-zero intervals tick at least once. */
    uint32_t ticks = (duration_ms + TIMER_TICK_MS - 1) / TIMER_TICK_MS;
    if (ticks == 0)
    {
        ticks = 1;
    }

    timer_counter[index] = ticks;
    timer_flag[index] = 0;
}

uint8_t isTimerExpired(uint8_t index)
{
    if (index >= TIMER_MAX)
    {
        return 0;
    }
    return (timer_flag[index] == 1) ? 1 : 0;
}

uint8_t isTimerExpiredAndClear(uint8_t index)
{
    if (index >= TIMER_MAX)
    {
        return 0;
    }
    if (timer_flag[index] == 1)
    {
        timer_flag[index] = 0;
        return 1;
    }
    return 0;
}

void clearTimer(uint8_t index)
{
    if (index >= TIMER_MAX)
    {
        return;
    }
    timer_counter[index] = 0;
    timer_flag[index] = 0;
}

void software_timer_run(void)
{
    for (uint8_t i = 0; i < TIMER_MAX; i++)
    {
        if (timer_counter[i] > 0)
        {
            timer_counter[i]--;
            if (timer_counter[i] == 0)
            {
                timer_flag[i] = 1;
            }
        }
    }
}

