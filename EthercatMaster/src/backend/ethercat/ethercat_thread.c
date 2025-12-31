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

		/* --- Synchronisation avec le monde PLC --- */
		int in_back = (atomic_load_i32(&d->active_in_buffer_idx) == 0) ? 1 : 0;
		int out_idx = atomic_load_i32(&d->rt_out_buffer_idx);

		// 1) SORTIES : buffers -> SOEM outputs
		for (int i = 0; i < d->slave_count; i++) {
			EtherCAT_Device_t* dev = d->slaves[i];
			if (dev->out_size > 0) {
				memcpy(dev->soem_outputs, dev->out_buffers[out_idx], dev->out_size);
			}
		}

		// 2) EtherCAT exchange
		ecx_send_processdata(&d->ctx);
		ecx_receive_processdata(&d->ctx, EC_TIMEOUTRET);

		// 3) ENTRÉES : SOEM inputs -> buffers
		for (int i = 0; i < d->slave_count; i++) {
			EtherCAT_Device_t* dev = d->slaves[i];
			if (dev->in_size > 0) {
				memcpy(dev->in_buffers[in_back], dev->soem_inputs, dev->in_size);
			}
		}

		// 4) Publication des entrées pour le PLC
		atomic_store_i32(&d->active_in_buffer_idx, in_back);


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

static OSAL_THREAD_HANDLE ecat_watchdog_thread(void* arg) {
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)arg;

	while (atomic_cas_i32(&d->running, 1, 1)) {
		// Vérifier que le cycle RT progresse
		uint64_t current_cycle = atomic_cas_i32(&d->stats.total_cycles);
		if (current_cycle == last_cycle) {
			printf("[WD] ALERTE: Thread RT bloqué!\n");
			atomic_fetch_add(&sys->errors_count, 1);
		}
		last_cycle = current_cycle;

		// Vérifier l'état des esclaves
		int16_t state = ecx_readstate(&d->ctx);
		for (int i = 1; i <= sys->ecx_context.slavecount; i++) {
			if (sys->ecx_context.slavelist[i].state != EC_STATE_OPERATIONAL) {
				atomic_store(&sys->system_state, state);

				printf("[WD] Esclave %d en erreur, état: 0x%02x\n",
					i, sys->ecx_context.slavelist[i].state);

				// Tentative de récupération
				sys->ecx_context.slavelist[i].state = EC_STATE_OPERATIONAL;
				ecx_writestate(&sys->ecx_context, i);
			}
		}

		/*if (prev_state != state) {
			atomic_store(&sys->system_state, state);
			printf("[WD] Changement d'état de Ethercat, state=0x%02x\n", state);
			prev_state = state;
		}*/

#if defined JITTER_CALC
		// Afficher les statistiques
		uint32_t jitter = atomic_load(&sys->max_jitter_us);
		uint32_t jitter_overflow = atomic_load(&sys->jitter_overflow_count);
		if (jitter > 100 || jitter_overflow > 0) {
			printf("[WD] Jitter max: %u us, overflow: %d\n", jitter, jitter_overflow);
		}
#endif

		Sleep(1000); // Vérification chaque seconde
	}
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
