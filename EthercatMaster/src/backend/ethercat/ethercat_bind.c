#include "ethercat_bind.h"
#include "ethercat_device_desc.h"
#include "ethercat_slave_runtime.h"
#include "app/plc_config.h"
#include <stdlib.h>

int ethercat_bind_device(EtherCAT_Driver_t* d, Device_t* dev, const DeviceConfig_t* cfg)
{
	uint16 pos = cfg->ethercat.expected_position;
	ec_slavet* sl = &d->ctx.slavelist[pos];

	const EtherCAT_DeviceDesc_t* desc =
		(const EtherCAT_DeviceDesc_t*)dev->desc->hw_desc;

	/* Vérification identité */
	if (sl->eep_man != desc->product_code ||
		sl->eep_id != desc->vendor_id) {
		printf("[ECAT] mismatch at pos %d\n", pos);
		return -1;
	}

	/* HOOK AVANT MAPPING */
	if (desc->config_hook) {
		if (desc->config_hook(&d->ctx, pos) < 0) {
			printf("[ECAT] hook failed at pos %d\n", pos);
			return -1;
		}
	}

	/* Enregistrement logique */
	EtherCAT_SlaveRuntime_t* rt = calloc(1, sizeof(*rt));
	if (rt == NULL) {
		printf("[ECAT] unable to allocate slave runtime\n");
		return -1;
	}

	rt->slave_index = pos;
	rt->device = dev;
	d->slaves[d->slave_count++] = rt;

	return 0;
}


