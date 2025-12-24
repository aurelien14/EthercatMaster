#pragma once
#include "ethercat_config.h"
#include "core/backend/backend.h"
#include "ethercat_slave_runtime.h"
#include <soem/soem.h>

#define ECAT_MAX_SLAVES 32

typedef struct {
	BackendDriver_t base;

	ecx_contextt ctx;
	uint8_t* iomap;
	size_t iomap_size;

	EtherCAT_SlaveRuntime_t* slaves[ECAT_MAX_SLAVES];
	size_t slave_count;

	volatile int running;
	uint32_t thread_cycle_us; 
	OSAL_THREAD_HANDLE thread;
	LARGE_INTEGER qpc_freq;
	LARGE_INTEGER next_deadline;
} EtherCAT_Driver_t;


EtherCAT_Driver_t* EtherCAT_Driver_Create(EtherCAT_config_t* config, int ethercat_index);
