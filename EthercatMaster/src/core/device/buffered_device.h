#pragma once
#include "device.h"
#include <stdint.h>

typedef struct {
    Device_t base;
    uint8_t* out_buffers[2];
    uint8_t* in_buffers[2];
    size_t out_size;
    size_t in_size;
} BufferedDevice_t;


int buffered_device_init(BufferedDevice_t* bdev, size_t in_size, size_t out_size);
void buffered_device_cleanup(BufferedDevice_t* bdev);



