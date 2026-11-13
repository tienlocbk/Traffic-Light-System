#ifndef FSM_TUNING_H_
#define FSM_TUNING_H_
#include <stdint.h>
void fsm_tuning_enter(void);
void fsm_tuning_run(uint8_t events);
uint8_t fsm_tuning_save(void);
#endif
