#include "device.h"
#include <stdint.h>


//TODO: mettre dans runtime : runtime_create_device
Device_t* device_create(DeviceConfig_t *cfg)
{
	Device_t* dev = cfg->device_desc->create(cfg);
	if (!dev)
		return NULL;

	dev->name = cfg->device_name;
	dev->plc_addr = cfg->plc_address;
	dev->desc = cfg->device_desc;

	return dev;
}




#if 0
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

static uint8_t* ethercat_device_get_rx_buffer(Device_t* dev)
{
	EtherCAT_Device_t* d = (EtherCAT_Device_t*)dev;
	EtherCAT_Driver_t* m = d->master;

	int idx = atomic_load_i32(&m->active_rx_idx);
	return d->rx_buffers[idx];
}


static uint8_t* ethercat_device_get_tx_buffer(Device_t* dev)
{
	EtherCAT_Device_t* d = (EtherCAT_Device_t*)dev;
	EtherCAT_Driver_t* m = d->master;

	int idx = atomic_load_i32(&m->active_tx_idx);
	return d->tx_buffers[idx];
#endif