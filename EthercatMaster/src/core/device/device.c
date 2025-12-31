#include "device.h"
#include <stdint.h>


Device_t* device_create(const DeviceConfig_t* cfg)
{
	if (!cfg || !cfg->device_desc || !cfg->device_desc->create)
		return NULL;

	Device_t* dev = cfg->device_desc->create(cfg);
	if (!dev)
		return NULL;

	dev->desc = cfg->device_desc;
	dev->plc_addr = cfg->plc_address;

	strncpy_s(dev->name, DEVICE_NAME_MAX - 1, cfg->device_name, DEVICE_NAME_MAX - 1);
	dev->name[DEVICE_NAME_MAX - 1] = '\0';

	dev->backend_handle = NULL;
	return dev;
}


void device_destroy(Device_t* dev)
{
	if(dev->desc->destroy)
		dev->desc->destroy(dev);
}