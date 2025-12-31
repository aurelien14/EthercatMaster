#pragma once
#include "core/device/buffered_device.h"

typedef struct EtherCAT_Driver EtherCAT_Driver_t;

typedef struct {
    BufferedDevice_t base;

    uint16_t slave_index;

    uint8_t* soem_outputs;
    uint8_t* soem_inputs;

    size_t rx_pdo_size;
    size_t tx_pdo_size;

} EtherCAT_Device_t;

