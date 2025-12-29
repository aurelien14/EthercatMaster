#pragma once
#include "device.h"
#include <stdint.h>

typedef struct {
    Device_t base;
    uint8_t* rx_buffers[2];
    uint8_t* tx_buffers[2];
    size_t rx_size;
    size_t tx_size;

} BufferedDevice_t;


int buffered_device_init(BufferedDevice_t* bdev, size_t rx_sz, size_t tx_sz);
void buffered_device_cleanup(BufferedDevice_t* bdev);

void buffered_device_swap_rx(Device_t* bdev);
uint8_t* device_get_input_ptr(Device_t* dev);
uint8_t* device_get_output_ptr(Device_t* dev);

