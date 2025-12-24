#pragma once

#include "core/device/device.h"
#include <stdint.h>

typedef struct {
	Device_t base;

	/* états métier L230 */
	float pt1_value;
	float pt2_value;
	float vc1_value;

	uint32_t status;
} L230_Device_t;

Device_t* L230_Create(void);
