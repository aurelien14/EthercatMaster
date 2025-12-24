#include "scheduler.h"
#include "scheduler_thread.h"

void scheduler_init(Scheduler_t* s, uint32_t base_cycle_us) {
	s->task_count = 0;
	s->base_cycle_us = base_cycle_us;
	InterlockedExchange(&s->running, 0);
	s->thread = NULL;
	QueryPerformanceFrequency(&s->qpc_freq);
	s->next_tick.QuadPart = 0;
}

void scheduler_add_task(Scheduler_t* s, PLC_Task_t* task) {
	if (s->task_count < 32) {
		s->tasks[s->task_count++] = *task;
	}
}


void scheduler_start(Scheduler_t* s) {
	scheduler_start_thread(s);
}

void scheduler_stop(Scheduler_t* s) {
	scheduler_stop_thread(s);
}