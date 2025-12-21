#include "plc_config.h"
#include "devices/L230/l230_device_desc.h"
//#include "devices/DAIO16/daio16_device_desc.h"
//#include "devices/DriveX/drive_modbus_desc.h"
#include "core/backend/backend.h"

/* Backends utilisés */
static const BackendConfig_t system_backends[] =
{
	{
		.type = PROTO_ETHERCAT,
		.name = "ec0",
		.ethercat = {
			.ifname = "\\Device\\NPF_{FB092E67-7CA1-4E8F-966D-AC090D396487}"
		}
	}
};

static const DeviceConfig_t system_devices[] = {
	{
		.device_name = "L230",
		.device_desc = &L230_DEVICE_DESC,
		.backend_name = "ec0",
		.ethercat = {
			.expected_position = 1
		}
	},
	{
		.device_name = "L230_2",
		.device_desc = &L230_DEVICE_DESC,
		.backend_name = "ec0",
		.ethercat = {
			.expected_position = 2
		}
	},

#if 0
	{
		.device_name = "ExtIO1",
		.device_desc = &DAIO16_DEVICE_DESC,
		.backend = BACKEND_ETHERCAT,
		.ethercat = {
			.expected_position = 2
		}
	},
	{
		.device_name = "Drive1",
		.device_desc = &DRIVE_X_MODBUS_DESC,
		.backend = BACKEND_MODBUS,
		.modbus = {
			.slave_id = 5
		}
	}
#endif

};


PLCSystemConfig_t plc_system_config = {
	.backends = (BackendConfig_t*)system_backends,
	.backend_count = sizeof(system_backends) / sizeof(system_backends[0]),
	.devices = (DeviceConfig_t*)system_devices,
	.device_count = sizeof(system_devices) / sizeof(system_devices[0])
};

