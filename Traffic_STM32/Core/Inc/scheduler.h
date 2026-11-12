#ifndef SCHEDULER_H_
#define SCHEDULER_H_
#include <stdint.h>
#define SCH_MAX_TASKS 10U
#define SCH_TICK_MS 10U
#define NO_TASK_ID UINT32_MAX
/* All delays/periods are ticks. Period 0 means one-shot.
 * Call Init before starting TIM2; Add/Delete/Dispatch from main context.
 * Tasks must return promptly and must not call Dispatch recursively.
 */
void SCH_Init(void);
uint32_t SCH_Add_Task(void (*task)(void), uint32_t delay, uint32_t period);
uint8_t SCH_Delete_Task(uint32_t id);
void SCH_Update(void);
void SCH_Dispatch_Tasks(void);
#endif
