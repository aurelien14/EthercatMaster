#include "backend/ethercat/ethercat.h"
#include "core/system/memalloc.h"
#include "l230_device.h"
#include "core/plateform/plateform.h"


Device_t* L230_Create(DeviceConfig_t* cfg)
{
    L230_Device_t* dev = (L230_Device_t*)CALLOC(1, sizeof(L230_Device_t));
    if (!dev) return NULL;

    return (Device_t*)dev;
}


void L230_Destroy(Device_t* dev) {
	L230_Device_t* d = (L230_Device_t*)dev;
	FREE(d);
}

