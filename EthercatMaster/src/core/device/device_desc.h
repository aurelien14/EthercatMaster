#pragma once
#include "config/protocol.h"

typedef struct Device Device_t;
typedef struct DeviceConfig DeviceConfig_t;

typedef struct DeviceDesc {
	const char* model;
	ProtocolType_t protocol;
	const void* hw_desc; 
	Device_t* (*create)(DeviceConfig_t* cfg);
	void (*destroy)(Device_t* dev);
	void (*update)(Device_t* dev);
	int (*is_operational)(Device_t* dev);
} DeviceDesc_t;