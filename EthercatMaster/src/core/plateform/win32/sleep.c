#include "sleep.h"

void wait_until_qpc(LARGE_INTEGER deadline, LARGE_INTEGER freq) {
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
				YieldProcessor();
			}
			break;
		}
	}
}