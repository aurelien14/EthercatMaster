#pragma once
#include "core/backend/backend.h"
#include "ethercat_slave_runtime.h"
#include <soem/soem.h>

typedef struct {
	BackendDriver_t base;

	ecx_contextt ctx;
	uint8_t* iomap;

	EtherCAT_SlaveRuntime_t* slaves[32];
	size_t slave_count;
} EtherCAT_Driver_t;

EtherCAT_Driver_t* EtherCAT_Driver_Create(const char* ifname);
