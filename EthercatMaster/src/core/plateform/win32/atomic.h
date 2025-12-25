#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude some conflicting definitions in windows header
#include <windows.h>
#include <stdint.h>

typedef volatile LONG atomic_i32_t;

static inline int32_t atomic_load_i32(const atomic_i32_t* v)
{
	return *v;
}

static inline void atomic_store_i32(atomic_i32_t* v, int32_t x)
{
	InterlockedExchange(v, x);
}

static inline int32_t atomic_exchange_i32(atomic_i32_t* v, int32_t x)
{
	return InterlockedExchange(v, x);
}

static inline int32_t atomic_cas_i32(
	atomic_i32_t* v,
	int32_t expected,
	int32_t desired)
{
	return InterlockedCompareExchange(v, desired, expected);
}
