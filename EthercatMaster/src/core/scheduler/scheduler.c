#include "scheduler.h"
#include "scheduler_thread.h"

void scheduler_init(Scheduler_t* s, Runtime_t* runtime, uint32_t base_cycle_us) {
	s->task_count = 0;
	s->base_cycle_us = base_cycle_us;
	atomic_store_i32(&s->running, 0);
	s->thread = NULL;
	//TODO: mettre un fonction commune win32, linux, plateform
	QueryPerformanceFrequency(&s->qpc_freq);
	s->next_tick.QuadPart = 0;
	s->runtime = runtime;
}


void scheduler_add_task_from_config(Scheduler_t* s, const PLC_TaskConfig_t* c)
{
	PLC_Task_t t = { 0 };

	t.name = c->name;
	t.period_ms = c->period_ms;
	t.offset_ms = c->offset_ms;
	t.run = c->run;
	t.policy = c->policy;

	// runtime precompute
	t.period_ns = (uint64_t)t.period_ms * 1000000ULL;
	t.offset_ns = (uint64_t)t.offset_ms * 1000000ULL;

	// init runtime
	t.init = false;
	t.overrun_count = 0;

	// Ajout
	if (s->task_count < 32) {
		s->tasks[s->task_count++] = t;
	}

}


void scheduler_add_task(Scheduler_t* s, PLC_Task_t* t) {
	if (s->task_count < 32) {
		t->period_ns = (uint64_t)t->period_ms * 1000000ULL;
		t->offset_ns = (uint64_t)t->offset_ms * 1000000ULL;
		s->tasks[s->task_count++] = *t;
	}
}


void scheduler_start(Scheduler_t* s) {
	scheduler_start_thread(s);
}

void scheduler_stop(Scheduler_t* s) {
	scheduler_stop_thread(s);
}