#include "ethercat.h"
#include "config/config.h"
#include "core/plateform/plateform.h"

#define EC_TIMEOUTMON 500

#define WD_BAD_TO_LATCH_FAULT	3	// ~30ms si WD à 10ms
#define WD_GOOD_TO_ARM			20	// ~200ms OK stable avant reset accepté


static void soem_lock(EtherCAT_Driver_t* d)
{
	while (!atomic_cas_i32(&d->soem_lock, 0, 1)) {
		// évite de brûler 100% CPU
		Sleep(0);
	}
}


static void soem_unlock(EtherCAT_Driver_t* d)
{
	atomic_store_i32(&d->soem_lock, 0);
}


static int ecat_is_in_op(ecx_contextt* ctx)
{
	// Variante simple : on considère "inOP" si on a des slaves
	// et que le master est déjà passé en OP au démarrage.
	// Si tu as un flag plus fiable (d->inOP), utilise-le.
	return (ctx && ctx->slavecount > 0);
}


static bool ecat_bus_ok(EtherCAT_Driver_t* d)
{
	// Conditions “saines”
	if (atomic_load_i32(&d->dowkccheck) != 0)
		return false;

	bool all_op = true;

	soem_lock(d);
	ecx_readstate(&d->ctx);
	for (int i = 1; i <= d->ctx.slavecount; i++) {
		ec_slavet* s = &d->ctx.slavelist[i];
		if (s->islost || s->state != EC_STATE_OPERATIONAL) {
			all_op = false;
			break;
		}
	}
	soem_unlock(d);

	return all_op;
}


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

	while (atomic_cas_i32(&d->running, 1, 1)) {

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
		soem_lock(d);
		ecx_send_processdata(&d->ctx);
		d->wkc = ecx_receive_processdata(&d->ctx, EC_TIMEOUTRET);
		ecx_mbxhandler(&d->ctx, 0, 4); //TODO: gérer limit = 4
		soem_unlock(d);

		if (d->wkc != d->expected_wkc)
			atomic_add_i32(&d->dowkccheck, 1);
		else atomic_store_i32(&d->dowkccheck, 0);


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

		/* --- Stats --- */
		Ethercat_Stats_t* s = &d->stats;

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


		if (cycle_us < s->min_cycle_time_us)
			s->min_cycle_time_us = cycle_us;
		if (cycle_us > s->max_cycle_time_us)
			s->max_cycle_time_us = cycle_us;

		if ((uint32_t)jitter_us < s->min_jitter_us)
			s->min_jitter_us = jitter_us;
		if ((uint32_t)jitter_us > s->max_jitter_us)
			s->max_jitter_us = jitter_us;

		s->jitter_us = jitter_us;
#endif
		s->total_cycles++;

		/* --- Next deadline --- */
		last = now;
		d->next_deadline.QuadPart += cycle_ticks;

		wait_until_qpc_2(d->next_deadline, d->qpc_freq);
	}

	return 0;
}




static OSAL_THREAD_HANDLE ecat_watchdog_thread(void* arg)
{
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)arg;
	const int currentgroup = 0;

	int bad_count = 0;
	int good_count = 0;

	printf("[WD] EtherCAT watchdog démarré (reset manuel)\n");

	while (atomic_cas_i32(&d->running, 1, 1))
	{
		// Etat "attendu" (démarré/armé) : on évite de lancer recovery si backend pas démarré
		int inOP_cmd = ecat_is_in_op(&d->ctx);

		// Symptôme WKC (mis à jour par le thread RT)
		int dowkccheck = atomic_load_i32(&d->dowkccheck);

		// docheckstate SOEM (optionnel)
		int docheck = 0;
		if (d->ctx.grouplist) {
			docheck = d->ctx.grouplist[currentgroup].docheckstate;
		}

		// Check "bus sain": (1) dowkccheck==0 (2) tous slaves OP et pas lost
		bool all_op = true;

		soem_lock(d);
		ecx_readstate(&d->ctx);

		for (int slaveix = 1; slaveix <= d->ctx.slavecount; slaveix++) {
			ec_slavet* s = &d->ctx.slavelist[slaveix];
			if (s->islost || s->state != EC_STATE_OPERATIONAL) {
				all_op = false;
				break;
			}
		}
		soem_unlock(d);

		bool bus_ok = (dowkccheck == 0) && all_op;

		// Hystérésis: défaut rapide / OK lent
		if (!bus_ok) {
			bad_count++;
			good_count = 0;
		}
		else {
			good_count++;
			bad_count = 0;
		}

		// --------------------------------------------------------------------
		// 1) Latch du défaut + inOP=0 (automatique)
		// --------------------------------------------------------------------
		// On déclenche la procédure SOEM-like seulement si :
		// - backend est "armé" (inOP_cmd)
		// - et (WKC mauvais persistant) ou docheckstate
		// - et défaut pas encore latché (sinon on évite de spammer)
		if (inOP_cmd && ((dowkccheck > 2) || docheck))
		{
			// On tente recovery SOEM-like, mais on laisse inOP revenir UNIQUEMENT par reset manuel
			soem_lock(d);

			if (d->ctx.grouplist) {
				d->ctx.grouplist[currentgroup].docheckstate = 0;
			}

			// IMPORTANT: readstate déjà fait plus haut sans lock, ici on le refait sous lock pour être cohérent
			ecx_readstate(&d->ctx);

			for (int slaveix = 1; slaveix <= d->ctx.slavecount; slaveix++)
			{
				ec_slavet* slave = &d->ctx.slavelist[slaveix];

				if (slave->state != EC_STATE_OPERATIONAL)
				{
					if (d->ctx.grouplist) {
						d->ctx.grouplist[currentgroup].docheckstate = 1;
					}

					// 1) SAFE_OP + ERROR => ACK
					if (slave->state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
					{
						printf("[WD] ERROR: slave %d SAFE_OP+ERROR, ack\n", slaveix);
						slave->state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
						ecx_writestate(&d->ctx, slaveix);
					}
					// 2) SAFE_OP => OP
					else if (slave->state == EC_STATE_SAFE_OP)
					{
						printf("[WD] WARN : slave %d SAFE_OP, request OP\n", slaveix);
						slave->state = EC_STATE_OPERATIONAL;

						if (slave->mbxhandlerstate == ECT_MBXH_LOST)
							slave->mbxhandlerstate = ECT_MBXH_CYCLIC;

						ecx_writestate(&d->ctx, slaveix);
					}
					// 3) > NONE => reconfig
					else if (slave->state > EC_STATE_NONE)
					{
						if (ecx_reconfig_slave(&d->ctx, slaveix, EC_TIMEOUTMON) >= EC_STATE_PRE_OP)
						{
							slave->islost = 0;
							printf("[WD] INFO : slave %d reconfigured\n", slaveix);
						}
					}
					// 4) NONE / pas encore lost => statecheck puis mark lost
					else if (!slave->islost)
					{
						ecx_statecheck(&d->ctx, slaveix, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);

						if (slave->state == EC_STATE_NONE)
						{
							slave->islost = 1;
							slave->mbxhandlerstate = ECT_MBXH_LOST;

							if (slave->Ibytes && slave->inputs) {
								memset(slave->inputs, 0x00, slave->Ibytes);
							}

							printf("[WD] ERROR: slave %d lost (state NONE)\n", slaveix);
						}
					}
				}

				// Si marqué lost => recover
				if (slave->islost)
				{
					if (slave->state <= EC_STATE_INIT)
					{
						if (ecx_recover_slave(&d->ctx, slaveix, EC_TIMEOUTMON))
						{
							slave->islost = 0;
							printf("[WD] INFO : slave %d recovered\n", slaveix);
						}
					}
					else
					{
						slave->islost = 0;
						printf("[WD] INFO : slave %d found\n", slaveix);
					}
				}
			}

			soem_unlock(d);

			// Clamp dowkccheck pour éviter overflow
			if (dowkccheck > 1000000) atomic_store_i32(&d->dowkccheck, 100);
		}

		// Si défaut persistant (bad_count), on latch et on coupe inOP
		if (bad_count >= WD_BAD_TO_LATCH_FAULT)
		{
			if (atomic_load_i32(&d->fault_latched) == 0) {
				atomic_store_i32(&d->fault_latched, 1);
				printf("[WD] FAULT LATCHED (dowkccheck=%d, all_op=%d)\n", dowkccheck, (int)all_op);
			}

			if (atomic_load_i32(&d->in_op) != 0) {
				atomic_store_i32(&d->in_op, 0);
				atomic_store_i32(&d->ethercat_state, EC_STATE_SAFE_OP); // ou état custom FAULT
				printf("[WD] inOP -> 0\n");
			}
		}

		// --------------------------------------------------------------------
		// 2) Réarmement MANUEL uniquement (reset_req)
		// --------------------------------------------------------------------
		if (atomic_load_i32(&d->fault_latched) != 0)
		{
			if (atomic_load_i32(&d->reset_req) != 0)
			{
				// On n'accepte le reset que si bus sain ET stable
				if (bus_ok && good_count >= WD_GOOD_TO_ARM)
				{
					atomic_store_i32(&d->fault_latched, 0);
					atomic_store_i32(&d->in_op, 1);
					atomic_store_i32(&d->ethercat_state, EC_STATE_OPERATIONAL);

					// Reset consommé
					atomic_store_i32(&d->reset_req, 0);

					// Quand tout est OK, on remet dowkccheck à 0
					atomic_store_i32(&d->dowkccheck, 0);

					printf("[WD] RESET accepté : inOP -> 1\n");
				}
				else
				{
					// Reset demandé mais bus pas stable/OK (log léger)
					static int spam = 0;
					if ((++spam % 100) == 0) {
						printf("[WD] RESET refusé : bus pas stable/OK (dowkccheck=%d, all_op=%d, good=%d)\n",
							dowkccheck, (int)all_op, good_count);
					}
				}
			}
		}
		else
		{
			// Si pas de défaut latché, on peut mettre inOP=1 seulement si tu le souhaites,
			// MAIS tu as demandé reset manuel => on laisse inOP piloté par start/stop + latch.
			// (Donc on ne fait rien ici)
		}

		// 10ms comme SOEM
		sleep_ms(10);
	}

	printf("[WD] EtherCAT watchdog arrêté\n");
	return 0;
}



void ethercat_start_thread(EtherCAT_Driver_t* d)
{
	atomic_exchange_i32(&d->running, 1);
	osal_thread_create_rt(&d->rt_thread, 128000, ethercat_thread, d);
	osal_thread_create(&d->watchdog_thread, 128000, ecat_watchdog_thread, d);
}


void ethercat_stop_thread(EtherCAT_Driver_t* d)
{
	atomic_exchange_i32(&d->running, 0);
	os_thread_join(d->rt_thread, NULL);
	os_thread_join(d->watchdog_thread, NULL);
}
