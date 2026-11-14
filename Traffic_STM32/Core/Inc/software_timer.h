#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#include <stdint.h>

#define TIMER_MAX       10
#define TIMER_TICK_MS   10

/* Timer indices for state machines */
#define TIMER_TRAFFIC   0
#define TIMER_DISPLAY   1
#define TIMER_BLINK     2
#define TIMER_TIMEOUT   3
#define TIMER_SENSOR    4

/* Flag array: timer_flag[i] = 1 when timer i expires */
extern int timer_flag[TIMER_MAX];

/* Reset all software timers */
void software_timer_init(void);

/* Set timer countdown in milliseconds */
void setTimer(uint8_t index, uint32_t duration_ms);

/* Check if timer has expired */
uint8_t isTimerExpired(uint8_t index);
uint8_t isTimerExpiredAndClear(uint8_t index);

/* Stop/clear a timer */
void clearTimer(uint8_t index);

/* Decrement timer counters, called every 10ms by scheduler */
void software_timer_run(void);

#define timer_run   software_timer_run
#define timer_init  software_timer_init

#endif /* INC_SOFTWARE_TIMER_H_ */
