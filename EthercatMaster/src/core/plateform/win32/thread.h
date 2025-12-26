#pragma once
#define WIN32_LEAN_AND_MEAN // Exclude some conflicting definitions in windows header
#include <windows.h>

int osal_thread_join(void* handle, void** value_ptr);