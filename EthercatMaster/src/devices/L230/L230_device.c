#include "l230_device.h"
#include "L230_device_desc.h"
#include <stdlib.h>
#include <string.h>

Device_t* L230_Create(void)
{
	L230_Device_t* dev = (L230_Device_t*)calloc(1, sizeof(L230_Device_t));
	if (!dev)
		return NULL;

	dev->base.name = "L230";
	dev->base.dev = dev;
	dev->base.desc = (DeviceDesc_t*)&L230_DEVICE_DESC;

	return &dev->base;
}

