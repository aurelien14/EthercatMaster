#pragma once
#include <osal.h>

static inline uint64_t qpc_to_us(const LARGE_INTEGER* delta, const LARGE_INTEGER* freq) {
	return (uint64_t)((delta->QuadPart * 1000000ULL) / freq->QuadPart);
}

void wait_until_qpc(LARGE_INTEGER deadline, LARGE_INTEGER freq);