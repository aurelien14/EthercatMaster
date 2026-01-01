#include "config/config.h"
#include "core/plateform/plateform.h"
#include "core/runtime/runtime.h"
#include "core/time/time.h"
#include "scheduler_thread.h"
#include <osal.h>

//TODO: configurer tick_period_ns deans config
static OSAL_THREAD_HANDLE scheduler_thread(void* arg)
{
	Scheduler_t* s = arg;
	Runtime_t* r = s->runtime;
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
			if (!t->init) {
				t->next_deadline = now;
				add_time_ns(&t->next_deadline, t->offset_ns);
				t->init = true;
			}

			if (osal_timespeccmp(&now, &t->next_deadline, >= )) {
				int health = atomic_load_i32(&s->health_level);

				bool allow = true;
				if (health >= PLC_HEALTH_FAULT) {
					switch (t->policy) {
					case PLC_TASK_ALWAYS_RUN:   allow = true;  break;
					case PLC_TASK_SKIP_ON_FAULT:allow = false; break;
					case PLC_TASK_CONTROL_ONLY: allow = false; break;
					default: allow = false; break;
					}
				}

				if (allow) {
					t->run(r);
					add_time_ns(&t->next_deadline, t->period_ns);
				}
			}
		}

		//synchronize buffers
		runtime_sync_backends(r);
	}

	timeEndPeriod(1);
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