#pragma once

#include "core/device/device.h"
#include "backend/ethercat/ethercat_slave_desc.h"

typedef struct {
	Device_t base;

	/* Descripteur protocolaire sélectionné */
	const EtherCAT_SlaveDesc_t* ecat_desc;

	/* PDO bindés (runtime) */
	struct L230_RX_PDO* rx;
	struct L230_TX_PDO* tx;

} L230_Device_t;

Device_t* L230_Create(void);
