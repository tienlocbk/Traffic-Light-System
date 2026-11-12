#include "scheduler.h"
#include "main.h"

typedef struct {
    void (*pTask)(void);
    uint32_t Delay, Period, TaskID;
    uint8_t RunMe;
} sTask;

static volatile sTask tasks[SCH_MAX_TASKS];
static volatile uint32_t count;
static uint32_t next_id;

static uint32_t Lock(void) {
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    return saved;
}
static void Unlock(uint32_t saved) { __set_PRIMASK(saved); }

/* Caller holds the lock. Subtract gaps instead of summing to avoid overflow. */
static void Insert(sTask task, uint32_t delay) {
    uint32_t i = 0;
    while (i < count && tasks[i].Delay <= delay) {
        delay -= tasks[i].Delay;
        i++;
    }
    for (uint32_t j = count; j > i; j--) tasks[j] = tasks[j - 1];
    task.Delay = delay;
    task.RunMe = (i == 0 && delay == 0);
    tasks[i] = task;
    if (i < count) tasks[i + 1].Delay -= delay;
    count++;
}

void SCH_Init(void) {
    uint32_t saved = Lock();
    count = 0;
    next_id = 0;
    Unlock(saved);
}

uint32_t SCH_Add_Task(void (*task)(void), uint32_t delay, uint32_t period) {
    uint32_t saved = Lock();
    /* Never recycle IDs: stale handles must not delete a different task. */
    if (!task || count == SCH_MAX_TASKS || next_id == NO_TASK_ID) {
        Unlock(saved);
        return NO_TASK_ID;
    }
    sTask t = {task, 0, period, next_id++, 0};
    Insert(t, delay);
    Unlock(saved);
    return t.TaskID;
}

uint8_t SCH_Delete_Task(uint32_t id) {
    uint32_t saved = Lock();
    uint32_t i = 0;
    while (i < count && tasks[i].TaskID != id) i++;
    if (i == count) { Unlock(saved); return 0; }
    /* Preserve the successor's deadline even when deleting a waiting head. */
    if (i + 1 < count) {
        tasks[i + 1].Delay += tasks[i].Delay;
        tasks[i + 1].RunMe = (i == 0 && tasks[i + 1].Delay == 0);
    }
    for (uint32_t j = i; j + 1 < count; j++) tasks[j] = tasks[j + 1];
    count--;
    Unlock(saved);
    return 1;
}

/* TIM2 ISR: constant work regardless of task count (lecture page 22).
 * Like the lecture, time freezes while a due head awaits dispatch.
 */
void SCH_Update(void) {
    if (!count) return;
    if (tasks[0].Delay) tasks[0].Delay--;
    if (!tasks[0].Delay) tasks[0].RunMe = 1;
}

void SCH_Dispatch_Tasks(void) {
    for (;;) {
        uint32_t saved = Lock();
        if (!count || !tasks[0].RunMe) { Unlock(saved); return; }
        sTask t = tasks[0];
        Unlock(saved);
        t.pTask(); /* Interrupts remain available while user code runs. */
        saved = Lock();
        /* Find by stable ID: the callback may have added/deleted tasks. */
        if (SCH_Delete_Task(t.TaskID) && t.Period) Insert(t, t.Period);
        Unlock(saved);
    }
}
