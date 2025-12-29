#pragma once
#include "device_desc.h"
#include "config/system_config.h"

typedef struct DeviceOps_t {
    void (*update)(Device_t* dev);
    bool (*is_operational)(Device_t* dev);
} DeviceOps_t;

typedef struct Device {
	const char* name;
	uint16_t plc_addr;
	BackendDriver_t* driver;
	const DeviceDesc_t* desc;
	const DeviceOps_t* ops; 
	void* backend_handle;
	void* protocol_runtime;
} Device_t;


Device_t* device_create(DeviceConfig_t* cfg);
