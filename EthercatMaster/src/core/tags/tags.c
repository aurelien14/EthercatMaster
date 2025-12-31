#include "config/system_config.h"
#include "tags.h"
#include "core/system/memalloc.h"
#include "config/config.h"
#include "core/backend/backend.h"
#include "core/device/device.h"
#include <stdio.h>



static const PLC_TypeInfo_t plc_type_info_array[] = {
	[PLC_BOOL] = {1, 1},
	[PLC_INT] = {2, 2},
	[PLC_DINT] = {2, 2},
	[PLC_REAL] = {4, 4},
	[PLC_STRING] = {PLC_TAG_STRING_SIZE, 4}, //TODO: voir si pointeur ou comment definir la taille
	[PLC_ARRAY] = {PLC_TAG_ARRAY_SIZE, 4},	//TODO: voir si pointeur ou comment definir la taille
};


static inline size_t plc_type_size(PLC_Tag_t *tag)
{
	return plc_type_info_array[tag->dtype].size;
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
	if (!tag || !out_value) return -1;

	if (tag->vtype == PLC_IN || tag->vtype == PLC_OUT) {
		Device_t* dev = tag->io.device;
		BackendDriver_t* drv = dev ? dev->driver : NULL;

		if (!drv || !drv->ops || !drv->ops->get_input_ptr) return -1;

		uint8_t* base = drv->ops->get_input_ptr(drv, dev);
		if (!base) return -1;

		uint8_t* addr = base + tag->io.offset;

		if (tag->dtype == PLC_BOOL) {
			uint8_t byte = *addr;
			*(uint8_t*)out_value = (byte >> tag->io.bit) & 1;
		}
		else {
			memcpy(out_value, addr, plc_type_size(tag));
		}
		return 0;
	}

}

int plc_tag_write(const PLC_Tag_t* tag, const void* in_value)
{
	if (!tag || !in_value)
		return -1;

	/* protection logique */
	if (tag->vtype == PLC_IN)
		return -1;


	switch (tag->vtype) {

	case PLC_OUT: {

		Device_t* dev = tag->io.device;
		BackendDriver_t* drv = dev ? dev->driver : NULL;

		if (!drv || !drv->ops || !drv->ops->get_output_ptr)
			return -1;

		uint8_t* base = (uint8_t*)drv->ops->get_output_ptr(drv, dev);
		if (!base)
			return -1;

		uint8_t* addr = base + tag->io.offset;

		if (tag->dtype == PLC_BOOL) {
			uint8_t v = (*(const uint8_t*)in_value) ? 1 : 0;
			uint8_t mask = (uint8_t)(1u << tag->io.bit);

			if (v) *addr |= mask;
			else   *addr &= (uint8_t)~mask;
		}
		else {
			size_t sz = plc_type_size(tag);
			if (sz == 0)
				return -1;
			memcpy(addr, in_value, sz);
		}
		return 0;
	}

	case PLC_HMI:
		/* ex: écrire dans un tableau hmi_values[tag->hmi.index] */
		return -1;

	case PLC_PV:
		/* ex: écrire dans une variable interne */
		return -1;
	}

	return -1;
}
