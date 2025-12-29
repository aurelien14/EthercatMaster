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

		/* --- Synchronisation avec le monde PLC --- */
		// 1. LES ENTRÉES (Esclave -> Master) : On remplit le "back_idx" pour le PLC
		int back_input = atomic_load_i32(&d->base.active_in_buffer_idx) == 0 ? 1 : 0;

		// 2. LES SORTIES (Master -> Esclave) : On prend ce que le PLC a validé
		int active_tx = atomic_load_i32(&d->base.active_out_buffer_idx);

		for (int i = 0; i < d->slave_count; i++) {
			EtherCAT_Device_t* dev = d->slaves[i];
			BufferedDevice_t* bdev = &dev->base;

			// --- ENTRÉES : SOEM -> rx_buffers ---
			// Les TX_PDO de l'esclave arrivent dans soem_inputs
			if (dev->in_size > 0) {
				memcpy(bdev->in_buffers[back_input], dev->soem_inputs, dev->in_size);
			}

			// --- SORTIES : tx_buffers -> SOEM ---
			// On envoie le buffer que le PLC est en train de NE PAS utiliser
			// (L'index active_tx est celui où le PLC écrit, on prend donc l'autre pour l'envoi)
			int out_idx = (active_tx == 0) ? 1 : 0;
			if (dev->out_size > 0) {
				memcpy(dev->soem_outputs, bdev->out_buffers[out_idx], dev->out_size);
			}
		}
		// On publie les nouvelles entrées pour le PLC
		atomic_store_i32(&d->base.active_out_buffer_idx, back_input);


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

