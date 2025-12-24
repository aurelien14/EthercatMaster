#pragma once
#include "core/device/device.h"
#include "ethercat.h"
#include "app/plc_config.h"

int ethercat_bind_device(EtherCAT_Driver_t* d, Device_t* dev, const DeviceConfig_t* cfg);
int ethercat_finalize_mapping(EtherCAT_Driver_t* ec);