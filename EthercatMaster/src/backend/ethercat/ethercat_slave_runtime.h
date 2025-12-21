#pragma once
#include "ethercat_slave_desc.h"

typedef struct {
	uint16_t slave_index;
	const EtherCAT_SlaveDesc_t* desc;

	void* rx_pdo;
	void* tx_pdo;
} EtherCAT_SlaveRuntime_t;
