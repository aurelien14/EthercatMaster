#pragma once
#include "device_desc.h"
#include "config/system_config.h"

typedef struct Device {
	const char* name;
	uint16_t plc_addr;
	const DeviceDesc_t* desc;
	void* backend_handle;
	void* protocol_runtime;
} Device_t;


Device_t* device_create(const DeviceConfig_t* cfg);
void* device_get_input_ptr(const Device_t* dev);
void* device_get_output_ptr(const Device_t* dev);