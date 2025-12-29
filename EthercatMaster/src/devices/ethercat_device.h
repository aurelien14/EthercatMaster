#pragma once
#include <stdint.h>
#include "core/device/device.h"

typedef struct {
	Device_t base;
	uint16_t slave_index;
	void* rx_pdo;
	void* tx_pdo;
} EtherCAT_Device_t;
