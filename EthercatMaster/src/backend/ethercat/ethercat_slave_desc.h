#pragma once
#include <stdint.h>
#include <stddef.h>
#include <soem/soem.h>


typedef int (*config_slave_hook)(ecx_contextt* ctx, uint16 slave);

typedef struct {
	uint32_t vendor_id;
	uint32_t product_code;

	size_t rx_pdo_size;
	size_t tx_pdo_size;

	config_slave_hook config_hook;

} EtherCAT_SlaveDesc_t;