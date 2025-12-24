#pragma once
#include <stdint.h>

typedef int (*PLC_TaskFunc)(void* ctx);

typedef struct {
	const char* name;

	uint32_t period_us;
	uint32_t offset_us;

	PLC_TaskFunc run;
	void* context;

	// runtime
	uint64_t next_deadline_us;
	uint64_t last_exec_us;
	uint64_t exec_time_us;
	uint64_t overrun_count;
} PLC_Task_t;
