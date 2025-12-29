#include "config/config.h"
#include "scheduler_thread.h"
#include "core/plateform/plateform.h"
#include "core/time/time.h"
#include <osal.h>

//a enlever
#include <stdio.h>
static OSAL_THREAD_HANDLE scheduler_thread(void* arg)
{
	Scheduler_t* s = arg;
	ec_timet next;
	const uint64_t tick_period_ns = 1000000; // Tick de 1ms

	// Force Windows à être plus précis (1ms)
	timeBeginPeriod(1);

	osal_get_monotonic_time(&next);

	while (atomic_cas_i32(&s->running, 1, 1)) {

		// 1. On avance l'échéance d'un pas fixe (évite la dérive)
		add_time_ns(&next, tick_period_ns);

		// 2. Attente jusqu'à l'échéance
		osal_monotonic_sleep(&next);

		// 3. Vérification des tâches
		ec_timet now;
		osal_get_monotonic_time(&now);

		for (size_t i = 0; i < s->task_count; i++) {
			PLC_Task_t* t = &s->tasks[i];

			// Initialisation au premier passage
			if (t->next_deadline.tv_sec == 0) t->next_deadline = now;

			if (osal_timespeccmp(&now, &t->next_deadline, >= )) {
				t->run(t->context);
				add_time_ns(&t->next_deadline, t->period_ns);
			}
		}
	}

	timeEndPeriod(1);
	return 0;
}
static OSAL_THREAD_HANDLE scheduler_thread_old(void* arg)
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
				//printf("[SCHEDULER] exec task %d\n", i);
				ec_timet t0, t1, dt;

				osal_get_monotonic_time(&t0);
				t->run(t->context);
				osal_get_monotonic_time(&t1);
				//t->init = 1;
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
};