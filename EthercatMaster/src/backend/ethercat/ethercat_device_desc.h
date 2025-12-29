#pragma once
#include <stdint.h>
#include <soem/soem.h>

typedef struct {
	uint32_t vendor_id;
	uint32_t product_code;

	size_t rx_pdo_size;
	size_t tx_pdo_size;

	int (*config_hook)(ecx_contextt* ctx, uint16_t slave);
} EtherCAT_DeviceDesc_t;
