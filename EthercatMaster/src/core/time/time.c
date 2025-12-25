#include "time.h"

/* add ns to ec_timet */
void add_time_ns(ec_timet* ts, int64 addtime)
{
	ec_timet addts;

	addts.tv_nsec = addtime % NSEC_PER_SEC;
	addts.tv_sec = (addtime - addts.tv_nsec) / NSEC_PER_SEC;
	osal_timespecadd(ts, &addts, ts);
}