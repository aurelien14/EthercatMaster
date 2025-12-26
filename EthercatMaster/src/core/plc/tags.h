#pragma once
#include "tags_desc.h"
#include "core/device/device.h"

typedef struct {
	const char* name;
	PLC_DataType_t dtype;
	PLC_VarType_t vtype;

	union {
		struct {
			Device_t* device;   // pointeur runtime
			uint16_t offset;
			uint8_t bit;
		} io;

		struct {
			uint32_t index;
		} hmi;

		struct {
			uint32_t index;
		} internal;
	};
} PLC_Tag_t;



PLC_Tag_t* create_plc_tags(size_t count);