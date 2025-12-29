#include "device.h"
#include <stdint.h>


//TODO: mettre dans runtime : runtime_create_device
Device_t* device_create(const DeviceConfig_t *cfg)
{
	Device_t* dev = cfg->device_desc->create(cfg);
	if (!dev)
		return NULL;

	dev->name = cfg->device_name;
	dev->plc_addr = cfg->plc_address;
	dev->desc = cfg->device_desc;

	
	return dev;
}


void* device_get_input_ptr(const Device_t* dev)
{
	if (!dev || !dev->desc || !dev->desc->get_input_ptr)
		return NULL;

	return dev->desc->get_input_ptr((Device_t*)dev);
}


void* device_get_output_ptr(const Device_t* dev)
{
	if (!dev || !dev->desc || !dev->desc->get_output_ptr)
		return NULL;

	return dev->desc->get_output_ptr((Device_t*)dev);
}
