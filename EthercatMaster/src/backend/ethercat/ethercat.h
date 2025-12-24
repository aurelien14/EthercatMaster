#pragma once
#include "ethercat_config.h"
#include "core/backend/backend.h"
#include "ethercat_slave_runtime.h"
#include <soem/soem.h>

#define ECAT_MAX_SLAVES 32	//TODO: mettre dans un fichier commun config.h

typedef struct Ethercat_Stats {
	uint32_t cycle_time_us;
	uint32_t min_cycle_time_us;
	uint32_t max_cycle_time_us;
	uint32_t jitter_us;
	uint32_t min_jitter_us;
	uint32_t max_jitter_us;
	uint64_t total_cycles;
} Ethercat_Stats_t;

typedef struct {
	BackendDriver_t base;

	ecx_contextt ctx;
	uint8_t* iomap;
	size_t iomap_size;

	EtherCAT_SlaveRuntime_t* slaves[ECAT_MAX_SLAVES];
	size_t slave_count;

	//thread ethercat
	volatile int running;
	uint32_t thread_cycle_us; //temps du cycle thread ethercat en microsecondes
	OSAL_THREAD_HANDLE thread;
	LARGE_INTEGER qpc_freq;
	LARGE_INTEGER next_deadline;

	Ethercat_Stats_t stats;
} EtherCAT_Driver_t;


EtherCAT_Driver_t* EtherCAT_Driver_Create(EtherCAT_config_t* config, int ethercat_index);
