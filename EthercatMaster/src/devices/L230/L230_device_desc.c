#include "l230_ethercat_desc.h"
#include "l230_device.h"

/* Description device générique */
const DeviceDesc_t L230_DEVICE_DESC = {
	.model = "L230",
	.protocol = PROTO_ETHERCAT,
	.hw_desc = &L230_ECAT_DESC,
	.create = L230_Create,
	.destroy = L230_Destroy,
	.is_operational = NULL // à implémenter plus tard
};