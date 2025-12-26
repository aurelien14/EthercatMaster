#include "ethercat.h"
#include "config/config.h"
#include "core/plateform/plateform.h"

#if defined PLATFORM_WINDOWS
static OSAL_THREAD_HANDLE ethercat_thread(void* arg)
{
	// Pin + priorité
	DWORD_PTR core_count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	DWORD_PTR mask = 1ULL << (core_count - 1);
	SetThreadAffinityMask(GetCurrentThread(), mask);

	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)arg;

	LARGE_INTEGER now, last;
	int64_t cycle_ticks;

	QueryPerformanceFrequency(&d->qpc_freq);

	cycle_ticks =
		(d->thread_cycle_us * d->qpc_freq.QuadPart) / 1000000LL;

	QueryPerformanceCounter(&d->next_deadline);
	last = d->next_deadline;

	while(atomic_cas_i32(&d->running, 1, 1)) {

		/* --- EtherCAT cycle --- */
		ecx_send_processdata(&d->ctx);
		ecx_receive_processdata(&d->ctx, EC_TIMEOUTRET);

		/* --- Mesures --- */
		QueryPerformanceCounter(&now);

		LARGE_INTEGER cycle_dt = {
			.QuadPart = now.QuadPart - last.QuadPart
		};

#if defined ETHERCAT_JITTER_CALC
		LARGE_INTEGER jitter_dt = {
			.QuadPart = now.QuadPart - d->next_deadline.QuadPart
		};

		uint32_t cycle_us =
			(uint32_t)((cycle_dt.QuadPart * 1000000LL) / d->qpc_freq.QuadPart);

		int32_t jitter_us =
			(int32_t)((jitter_dt.QuadPart * 1000000LL) / d->qpc_freq.QuadPart);

		if (jitter_us < 0)
			jitter_us = -jitter_us;

		/* --- Stats --- */
		Ethercat_Stats_t* s = &d->stats;

		if (cycle_us < s->min_cycle_time_us)
			s->min_cycle_time_us = cycle_us;
		if (cycle_us > s->max_cycle_time_us)
			s->max_cycle_time_us = cycle_us;

		if ((uint32_t)jitter_us < s->min_jitter_us)
			s->min_jitter_us = jitter_us;
		if ((uint32_t)jitter_us > s->max_jitter_us)
			s->max_jitter_us = jitter_us;

		s->jitter_us = jitter_us;
		s->total_cycles++;
#endif
		/* --- Next deadline --- */
		last = now;
		d->next_deadline.QuadPart += cycle_ticks;

		wait_until_qpc_2(d->next_deadline, d->qpc_freq);
	}

	return 0;
}
#else
static OSAL_THREAD_HANDLE ethercat_thread(void* arg)
{

}
#endif

void ethercat_start_thread(EtherCAT_Driver_t* d)
{
	atomic_exchange_i32(&d->running, 1);
	osal_thread_create_rt(&d->thread, 128000, ethercat_thread, d);
}


void ethercat_stop_thread(EtherCAT_Driver_t* d)
{
	atomic_exchange_i32(&d->running, 0);
	os_thread_join(d->thread, NULL);
}

