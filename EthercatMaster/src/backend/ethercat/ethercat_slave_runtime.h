#pragma once
#include <stdint.h>
#include "core/device/device.h"

typedef struct {
	uint16_t slave_index;
	void* rx_pdo;
	void* tx_pdo;

	Device_t* device;
} EtherCAT_SlaveRuntime_t;
