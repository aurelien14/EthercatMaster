#include "config/config.h"
#include "scheduler_thread.h"
#include "core/plateform/plateform.h"
#include "core/time/time.h"
#include <osal.h>

static OSAL_THREAD_HANDLE scheduler_thread(void* arg)
{
	Scheduler_t* s = arg;

	ec_timet next;
	ec_timet now;

	osal_get_monotonic_time(&next);
	/* alignement optionnel */
	next.tv_nsec = ((next.tv_nsec / 1000000) + 1) * 1000000;

	while (atomic_cas_i32(&s->running, 1, 1)) {
		/* attendre le tick */
		osal_monotonic_sleep(&next);

		osal_get_monotonic_time(&now);

		for (size_t i = 0; i < s->task_count; i++) {
			PLC_Task_t* t = &s->tasks[i];

			if (osal_timespeccmp(&now, &t->next_deadline, >= )) {

				ec_timet t0, t1, dt;

				osal_get_monotonic_time(&t0);
				t->run(t->context);
				osal_get_monotonic_time(&t1);
				t->init = 1;
				osal_time_diff(&t0, &t1, &dt);

				t->exec_time_ns =
					dt.tv_sec * 1000000000ULL + dt.tv_nsec;

				if (t->exec_time_ns > t->period_ns)
					t->overrun_count++;

				add_time_ns(&t->next_deadline, t->period_ns);
			}
		}
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
	os_thread_join(s->thread, NULL);
}