#include "config/config.h"
#include "scheduler_thread.h"
#include "core/plateform/plateform_detect.h"
#include "utils/threads_utils.h"

static OSAL_THREAD_HANDLE scheduler_thread(void* arg)
{
	Scheduler_t* s = arg;
	LARGE_INTEGER now;

	QueryPerformanceFrequency(&s->qpc_freq);
	QueryPerformanceCounter(&s->next_tick);

	const int64_t base_ticks =
		(s->base_cycle_us * s->qpc_freq.QuadPart) / 1000000LL;

	while (atomic_cas_i32(&s->running, 1, 1)) {

		QueryPerformanceCounter(&now);

		uint64_t now_us =
			(now.QuadPart * 1000000LL) / s->qpc_freq.QuadPart;

		for (size_t i = 0; i < s->task_count; i++) {
			PLC_Task_t* t = &s->tasks[i];

			if (now_us >= t->next_deadline_us) {

				LARGE_INTEGER t0, t1;
				QueryPerformanceCounter(&t0);

				t->run(t->context);

				QueryPerformanceCounter(&t1);

				uint64_t exec_us =
					(t1.QuadPart - t0.QuadPart) * 1000000LL / s->qpc_freq.QuadPart;

				t->exec_time_us = exec_us;
				t->last_exec_us = now_us;

				if (exec_us > t->period_us)
					t->overrun_count++;

				t->next_deadline_us += t->period_us;
			}
		}

		s->next_tick.QuadPart += base_ticks;
		wait_until_qpc(s->next_tick, s->qpc_freq);
	}

	return 0;
}


void scheduler_start_thread(Scheduler_t* s)
{
	atomic_exchange_i32(&s->running, 1);
	osal_thread_create(&s->thread, 128000, scheduler_thread, s);
}


void scheduler_stop_thread(Scheduler_t* s)
{
	atomic_exchange_i32(&s->running, 0);
	WaitForSingleObject(s->thread, INFINITE);
}