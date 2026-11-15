#ifndef FSM_MODE_H_
#define FSM_MODE_H_
#include <stdint.h>

/* Call once before registering fsm_mode_run every FSM_PERIOD_MS. */
void fsm_mode_init(void);
/* Feed debounced press edges from a scheduler task, never raw levels or an ISR.
 * Events are consumed once per run, including ignored lower-priority events. */
void fsm_mode_post_buttons(uint8_t events);
void fsm_mode_run(void);
#endif
