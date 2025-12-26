#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <osal.h>

typedef int (*PLC_TaskFunc)(void* ctx);

typedef struct {
	const char* name;

	uint32_t period_ms;
	uint32_t offset_ms;

	PLC_TaskFunc run;
	void* context;

	/* runtime */
	bool init;
	uint64_t period_ns;
	uint64_t offset_ns;

	ec_timet next_deadline;
	uint64_t last_exec_ns;
	uint64_t exec_time_ns;
	uint64_t overrun_count;
} PLC_Task_t;


typedef struct {
	PLC_Task_t* pTask;

} PLC_TaskContext_t;