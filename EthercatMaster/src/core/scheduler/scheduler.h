#pragma once

#include "task.h"
#include "config/config.h"
#include <osal.h>

typedef struct {
	PLC_Task_t tasks[PLC_MAX_TASKS];
	size_t task_count;

	uint32_t base_cycle_us;
	volatile LONG running; //for scheduler thread, private

	LARGE_INTEGER qpc_freq;
	LARGE_INTEGER next_tick;
	HANDLE thread;
} Scheduler_t;


void scheduler_init(Scheduler_t* s, uint32_t base_cycle_us);
void scheduler_add_task(Scheduler_t* s, PLC_Task_t* task);
void scheduler_start(Scheduler_t* s);
void scheduler_stop(Scheduler_t* s);