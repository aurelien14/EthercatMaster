#include "l230_device.h"
#include "core/system/memalloc.h"
#include <string.h>


Device_t* L230_Create(DeviceConfig_t* cfg)
{
	L230_Device_t* dev = (L230_Device_t*)CALLOC(1, sizeof(L230_Device_t));
	if (!dev)
		return NULL;

	return (Device_t*)dev;
}

void L230_Destroy(Device_t* dev) {
	L230_Device_t* d = (L230_Device_t*)dev;
	FREE(d);
}

void* L230_get_input_ptr(Device_t* dev) {
	L230_Device_t* d = (L230_Device_t*)dev;
	return d->ec_dev.tx_pdo;
}

void* L230_get_output_ptr(Device_t* dev) {
	L230_Device_t* d = (L230_Device_t*)dev;
	return d->ec_dev.rx_pdo;
}