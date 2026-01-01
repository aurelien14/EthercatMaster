#pragma once
#include "plateform_detect.h"

#if defined PLATFORM_WINDOWS
#include "win32/atomic.h"
#include "win32/thread.h"
#include "win32/sleep.h"
#include <timeapi.h>
#endif

//void sleep_ms(unsigned long ms);