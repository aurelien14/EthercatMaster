#pragma once
#include "device_desc.h"

typedef struct Device {
	const char* name;
	const DeviceDesc_t* desc;
	void* backend_handle;
	void* protocol_runtime;
	void* dev;
} Device_t;