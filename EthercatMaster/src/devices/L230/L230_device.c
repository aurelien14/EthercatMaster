#include "backend/ethercat/ethercat.h"
#include "core/system/memalloc.h"
#include "l230_device.h"
#include "core/plateform/plateform.h"

static const DeviceOps_t L230_OPS;


Device_t* L230_Create(DeviceConfig_t* cfg)
{
    L230_Device_t* dev = (L230_Device_t*)CALLOC(1, sizeof(L230_Device_t));
    if (!dev) return NULL;

    if (ethercat_device_init(&dev->ec_dev, cfg) != 0) {
        FREE(dev);
        return NULL;
    }

    dev->ec_dev.base.base.ops = &L230_OPS;
    return (Device_t*)dev;
}


void L230_Destroy(Device_t* dev) {
	L230_Device_t* d = (L230_Device_t*)dev;
	FREE(d);
}


void L230_update(Device_t* dev) {

}

static const DeviceOps_t L230_OPS = {
    .update = L230_update,
    .is_operational = NULL // à implémenter plus tard
};