#pragma once
#include "core/device/device.h"

typedef struct EtherCAT_Driver EtherCAT_Driver_t;

typedef struct EtherCAT_Device {
    Device_t base;

    uint16_t slave_index;

    uint8_t* soem_outputs;
    uint8_t* soem_inputs;

    //double buffuring
    uint8_t* out_buffers[2];
    uint8_t* in_buffers[2];
    size_t out_size;
    size_t in_size;


    size_t rx_pdo_size;
    size_t tx_pdo_size;

} EtherCAT_Device_t;

