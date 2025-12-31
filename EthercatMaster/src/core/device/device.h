#pragma once
#include "device_desc.h"
#include "config/system_config.h"

#define DEVICE_NAME_MAX	64

typedef struct Device {
	char name[DEVICE_NAME_MAX];
	uint16_t plc_addr;
	BackendDriver_t* driver;
	const DeviceDesc_t* desc;
	void* backend_handle;
	void* protocol_runtime;
} Device_t;


Device_t* device_create(DeviceConfig_t* cfg);
void device_destroy(Device_t* dev);