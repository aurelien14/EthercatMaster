#include "L230_device_desc.h"
#include "L230_ethercat_desc.h"
#include "L230_device.h"

static const DeviceProtocolDesc_t l230_protocols[] = {
	{
		.protocol = PROTO_ETHERCAT,
		.protocol_desc = &L230_ECAT_DESC
	}
};

const DeviceDesc_t L230_DEVICE_DESC = {
	.model = "L230",
	.create = L230_Create,
	.protocols = l230_protocols,
	.protocol_count = 1
};
