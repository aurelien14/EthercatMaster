#pragma once

#include "devices/ethercat_device.h"
#include <stdint.h>

typedef struct {
	EtherCAT_Device_t ec_dev;

	/* états métier L230 */
	float pt1_value;
	float pt2_value;
	float vc1_value;

	uint32_t status;
} L230_Device_t;

Device_t* L230_Create(DeviceConfig_t* cfg);
void L230_Destroy(Device_t* dev);
void* L230_get_input_ptr(Device_t* dev);
void* L230_get_output_ptr(Device_t* dev);