#pragma once
#include "tags_desc.h"

typedef struct Device Device_t;

typedef struct {
	size_t size;
	size_t align;
} PLC_TypeInfo_t;


typedef struct {
	const char* name;
	PLC_DataType_t dtype;
	PLC_VarType_t vtype;

	union {
		struct {
			Device_t* device;
			uint16_t offset;
			uint8_t  bit;
		} io;

		struct {
			uint32_t index;   // index dans table HMI
		} hmi;

		struct {
			uint32_t index;   // variables internes
		} internal;
	};
} PLC_Tag_t;




PLC_Tag_t* create_plc_tags(size_t count);
int plc_tag_read(const PLC_Tag_t* tag, void* out_value);
int plc_tag_write(const PLC_Tag_t* tag, const void* in_value);