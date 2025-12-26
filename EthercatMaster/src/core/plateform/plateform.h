#pragma once
#include "plateform_detect.h"

#if defined PLATFORM_WINDOWS
#include "win32/atomic.h"
#include "win32/thread.h"
#include "win32/sleep.h"
#endif

