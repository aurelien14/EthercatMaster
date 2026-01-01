#pragma once

#include "task.h"
#include "config/config.h"
#include "config/system_config.h"
#include "core/plateform/plateform.h"

typedef struct Runtime Runtime_t;

typedef enum {
	PLC_HEALTH_OK = 0,
	PLC_HEALTH_WARN = 1,
	PLC_HEALTH_FAULT = 2,
} PLC_Health;


typedef enum {
	PLC_FAULT_NONE = 0,
	PLC_FAULT_BACKEND_LOST = 1u << 0,
	PLC_FAULT_DEVICE_LOST = 1u << 1,
	PLC_FAULT_ECAT_NOT_OP = 1u << 2,
} PLC_Fault_Flags_t;


typedef struct {
	PLC_Task_t tasks[PLC_MAX_TASKS];
	size_t task_count;

	Runtime_t* runtime;

	uint32_t base_cycle_us;
	volatile LONG running; //for scheduler thread, private

	LARGE_INTEGER qpc_freq;
	LARGE_INTEGER next_tick;
	HANDLE thread;

	atomic_i32_t health_level;
	atomic_i32_t fault_flags;
} Scheduler_t;


void scheduler_init(Scheduler_t* s, Runtime_t* runtime, uint32_t base_cycle_us);
void scheduler_add_task_from_config(Scheduler_t* s, const PLC_TaskConfig_t* c);
void scheduler_add_task(Scheduler_t* s, PLC_Task_t* task);
void scheduler_start(Scheduler_t* s);
void scheduler_stop(Scheduler_t* s);