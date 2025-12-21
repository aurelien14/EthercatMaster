#pragma once

#include "device_protocol_desc.h"

typedef struct Device Device_t;

/* Description complète d’un modèle de device */
typedef struct {
	const char* model;
	Device_t* (*create)(void);
	const DeviceProtocolDesc_t* protocols;
	size_t protocol_count;
} DeviceDesc_t;