#pragma once
#include <stdint.h>
#include <soem/soem.h>

typedef int (*EtherCAT_ConfigHook)(void* master, uint16_t slave_index);

typedef struct {
	uint32_t vendor_id;
	uint32_t product_code;
	size_t   rx_pdo_size;
	size_t   tx_pdo_size;
	EtherCAT_ConfigHook config_hook;
} EtherCAT_DeviceDesc_t;