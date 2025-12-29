#include "config/system_config.h"
#include "core/device/buffered_device.h"
#include "tags.h"
#include "core/system/memalloc.h"
#include "config/config.h"
#include <stdio.h>



static const PLC_TypeInfo_t plc_type_info_array[] = {
	[PLC_BOOL] = {1, 1},
	[PLC_INT] = {2, 2},
	[PLC_DINT] = {2, 2},
	[PLC_REAL] = {4, 4},
	[PLC_STRING] = {PLC_TAG_STRING_SIZE, 4}, //TODO: voir si pointeur ou comment definir la taille
	[PLC_ARRAY] = {PLC_TAG_ARRAY_SIZE, 4},	//TODO: voir si pointeur ou comment definir la taille
};


static inline size_t plc_type_size(PLC_DataType_t type)
{
	return plc_type_info_array[type].size;
}



PLC_Tag_t* create_plc_tags(size_t count) {
	PLC_Tag_t* t = CALLOC(1, count * sizeof(PLC_Tag_t));
	if (t == NULL) {
		printf("Error to create iomap tags\n");
	}
	return t;
}


int plc_tag_read(const PLC_Tag_t* tag, void* out_value)
{
	if (!tag || !out_value)
		return -1;

	switch (tag->vtype) {
	case PLC_IN: {
		uint8_t* base = device_get_input_ptr(tag->io.device);
		if (base == NULL)
			break;
		uint8_t* addr = base + tag->io.offset;

		if (tag->dtype == PLC_BOOL) {
			*(bool*)out_value = (*addr >> tag->io.bit) & 1;
		}
		else {
			memcpy(out_value, addr, plc_type_size(tag->dtype));
		}
		return 0;
	}

	case PLC_HMI:
		return 0;// hmi_read(tag->hmi.index, out_value);

	case PLC_PV:
		return 0;// internal_read(tag->internal.index, out_value);
	}


	return -1;
}


int plc_tag_write(const PLC_Tag_t* tag, int in_value)
{
	if (!tag)
		return -1;

	switch (tag->vtype) {
	case PLC_OUT: {
		uint8_t* base = device_get_input_ptr(tag->io.device);
		if (base == NULL)
			break;
		uint8_t* addr = base + tag->io.offset;

		if (tag->dtype == PLC_BOOL) {
			if (in_value)
				*addr |= (1 << tag->io.bit);
			else
				*addr &= ~(1 << tag->io.bit);
		}
		else {
			//memcpy(addr, in_value, plc_type_size(tag->dtype));
		}
		return 0;
	}

	case PLC_HMI:
		return 0;// hmi_write(tag->hmi.index, in_value);

	case PLC_PV:
		return 0;// internal_write(tag->internal.index, in_value);
	}

	return -1;
}
