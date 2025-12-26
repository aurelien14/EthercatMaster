#pragma once
#include "core/device/device.h"
#include "config/system_config.h"
#include "ethercat.h"

int ethercat_bind_device(EtherCAT_Driver_t* d, Device_t* dev, const DeviceConfig_t* cfg);
int ethercat_finalize_mapping(EtherCAT_Driver_t* ec);