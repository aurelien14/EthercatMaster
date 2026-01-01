#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <osal.h>

typedef int (*PLC_TaskFunc)(void* ctx);

typedef enum {
	PLC_TASK_ALWAYS_RUN = 0,   // safety, diag, hmi, recovery
	PLC_TASK_SKIP_ON_FAULT,    // logique non critique
	PLC_TASK_CONTROL_ONLY,     // commande machine -> stop si fault
} PLC_Task_Policy_t;


typedef struct {
	const char* name;

	uint32_t period_ms;
	uint32_t offset_ms;

	PLC_TaskFunc run;
	
	PLC_Task_Policy_t policy;

	/* runtime */
	bool init;
	uint64_t period_ns;
	uint64_t offset_ns;

	ec_timet next_deadline;
	uint64_t last_exec_ns;
	uint64_t exec_time_ns;
	uint64_t overrun_count;
} PLC_Task_t;
