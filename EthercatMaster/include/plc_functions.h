#pragma once
#include <stdint.h>
#include "plc_task_manager.h"


typedef struct PLC_timer {
	bool active = 0;
	uint64_t start_time_ms;
	uint32_t preset_ms;
	bool output;
} PLC_timer_t;

bool plc_ton(PLCTask_t* self, PLC_timer* t, bool input, uint32_t preset_ms);
bool plc_tof(PLCTask_t* self, PLC_timer* t, bool input, uint32_t preset_ms);