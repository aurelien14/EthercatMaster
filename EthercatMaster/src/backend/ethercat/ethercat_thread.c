#include "ethercat.h"
#include <intrin.h>

static void wait_until_qpc(LARGE_INTEGER deadline, LARGE_INTEGER freq)
{
	LARGE_INTEGER now;
	int64_t remaining_us;

	for (;;) {
		QueryPerformanceCounter(&now);

		remaining_us =
			(deadline.QuadPart - now.QuadPart) * 1000000 / freq.QuadPart;

		if (remaining_us <= 0)
			break;

		if (remaining_us > 200) {
			Sleep(1);   // gros morceau
		}
		else if (remaining_us > 50) {
			Sleep(0);   // yield
		}
		else {
			// busy-wait court
			while (QueryPerformanceCounter(&now),
				now.QuadPart < deadline.QuadPart) {
				_mm_pause();
			}
			break;
		}
	}
}


static OSAL_THREAD_FUNC ethercat_thread(void* arg)
{
	EtherCAT_Driver_t* d = arg;

	const int64_t cycle_ticks =
		(d->thread_cycle_us * d->qpc_freq.QuadPart) / 1000000;

	QueryPerformanceCounter(&d->next_deadline);

	while (d->running) {

		ecx_send_processdata(&d->ctx);
		ecx_receive_processdata(&d->ctx, EC_TIMEOUTRET);

		d->next_deadline.QuadPart += cycle_ticks;

		wait_until_qpc(d->next_deadline, d->qpc_freq);
	}

	return 0;
}


void ethercat_start_thread(EtherCAT_Driver_t* d)
{
	InterlockedExchange(&d->running, 1);
	osal_thread_create_rt(&d->thread, 128000, ethercat_thread, d);
}


void ethercat_stop_thread(EtherCAT_Driver_t* d)
{
	_InterlockedExchange(&d->running, 0);
	WaitForSingleObject(d->thread, INFINITE);
}