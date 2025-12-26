#define WIN32_LEAN_AND_MEAN // Exclude some conflicting definitions in windows header
#include <windows.h>

int os_thread_join(void* thandle, void** value_ptr)
{
	DWORD retvalue = WaitForSingleObject(thandle, INFINITE);
	if (retvalue == WAIT_OBJECT_0) {
		return 0;
	}
	else {
		return EINVAL;
	}
}