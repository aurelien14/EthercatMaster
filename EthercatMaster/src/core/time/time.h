#pragma once
#include <osal.h>


#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000LL
#endif

#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000LL
#endif

void add_time_ns(ec_timet* ts, int64 addtime);