#pragma once

#include "device_desc.h"

/* Device runtime (instance dans le système) */
typedef struct Device {
	const char* name;           /* "MainIO", "ExtIO1" */
	const DeviceDesc_t* desc;   /* L230_DEVICE_DESC */
	ProtocolType_t protocol;        /* protocole réellement utilisé */

	void* backend_handle;       /* pointeur opaque vers le backend */
	void* userdata;             /* pointeur vers struct spécifique (L230_Device_t*) */
} Device_t;


const DeviceProtocolDesc_t*
device_find_protocol_desc(const DeviceDesc_t* desc, ProtocolType_t proto);
