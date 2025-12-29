#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "config/system_config.h"
#include "core/runtime/runtime.h"

//TODO: make main independant plateform

static Runtime_t* p_runtime = NULL;
static LONG g_timer_users = 0;

extern const PLCSystemConfig_t PLC_SYSTEM_CONFIG;

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
		if (p_runtime) {
			printf("\n[MAIN] Arrêt demandé\n");
			InterlockedExchange(&p_runtime->system_is_running, 0);
		}
		return TRUE;
	}
	return FALSE;
}

static void rt_timer_acquire(void)
{
	if (InterlockedIncrement(&g_timer_users) == 1)
		timeBeginPeriod(1);
}

static void rt_timer_release(void)
{
	if (InterlockedDecrement(&g_timer_users) == 0)
		timeEndPeriod(1);
}


int main(int argc, char* argv[])
{
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
	rt_timer_acquire();

	p_runtime = create_runtime();
	if (!p_runtime)
		return -1;

	InterlockedExchange(&p_runtime->system_is_running, 1);

	if (runtime_init(p_runtime, &PLC_SYSTEM_CONFIG) < 0)
		return -1;

	runtime_start(p_runtime);


	while (InterlockedCompareExchange(&p_runtime->system_is_running, 1, 1)) {
		runtime_process(p_runtime);
		Sleep(1000); // Toutes les 5 secondes
	}

	runtime_stop(p_runtime);
	runtime_cleanup(p_runtime);
	rt_timer_release();
	return 0;
}
