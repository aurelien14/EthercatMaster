#pragma once
#define WIN32_LEAN_AND_MEAN // Exclude some conflicting definitions in windows header
#include <windows.h>
#include <stdint.h>

static inline uint64_t qpc_to_us(const LARGE_INTEGER* delta, const LARGE_INTEGER* freq) {
	return (uint64_t)((delta->QuadPart * 1000000ULL) / freq->QuadPart);
}


static inline void wait_until_qpc(LARGE_INTEGER deadline, LARGE_INTEGER freq) {
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


static inline void wait_until_qpc_2(LARGE_INTEGER deadline, LARGE_INTEGER freq)
{
	LARGE_INTEGER now;
	int64_t remaining_us;

	for (;;) {
		QueryPerformanceCounter(&now);

		remaining_us =
			(deadline.QuadPart - now.QuadPart) * 1000000LL / freq.QuadPart;

		if (remaining_us <= 0)
			break;

		if (remaining_us > 1000) {
			Sleep(1);
		}
		else if (remaining_us > 100) {
			Sleep(0);
		}
		else {
			/* --- Final spin --- */
			do {
				YieldProcessor();
				QueryPerformanceCounter(&now);
			} while (now.QuadPart < deadline.QuadPart);
			break;
		}
	}
}