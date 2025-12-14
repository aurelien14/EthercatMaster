#pragma once
#include <stdint.h>
#include "plc_task_manager.h"


typedef struct PLC_timer {
	bool active = 0;
	uint64 start_time_ms;
	uint32 preset_ms;
	bool output;
} PLC_timer_t;

typedef struct PLC_pulse {
	PLC_timer_t* t1;
	PLC_timer_t* t2;
	uint32 period_ms;
	float32 freq;
} PLC_pulse_t;

typedef struct PLC_ramp {
	bool enable = 0;
	uint64_t start_time_ms;
	uint32_t slop;
} PLC_ramp_t;

boolean plc_ton(PLCTask_t* self, PLC_timer_t* t, boolean input, uint32_t preset_ms);
boolean plc_tof(PLCTask_t* self, PLC_timer_t* t, boolean input, uint32_t preset_ms);

int ramp(PLCTask_t* self, PLC_timer_t* t, boolean input, uint32_t slope);

float32 pt100_resistance_to_temperature(float32 resistance);
float32 pt100_resistance_to_temperature_simple(float32 resistance);