#include "core/runtime/runtime.h"
#include "tags.h"
#include "core/system/memalloc.h"
#include "config/config.h"
#include "core/backend/backend.h"
#include "core/device/device.h"
#include <stdio.h>
#include <stdlib.h>


static const PLC_TypeInfo_t plc_type_info_array[] = {
	[PLC_BOOL] = {1, 1},
	[PLC_INT] = {2, 2},
	[PLC_DINT] = {4, 4},
	[PLC_WORD] = {2, 2},
	[PLC_DWORD] = {4, 4},
	[PLC_REAL] = {4, 4},
	[PLC_STRING] = {PLC_TAG_STRING_SIZE, 4}, //TODO: voir si pointeur ou comment definir la taille
	[PLC_ARRAY] = {PLC_TAG_ARRAY_SIZE, 4},	//TODO: voir si pointeur ou comment definir la taille
};


static inline size_t plc_type_size(PLC_Variable_t*tag)
{
	return plc_type_info_array[tag->dtype].size;
}


PLC_Variable_t* get_tag_by_index(Runtime_t* runtime, size_t index) {
	if (runtime->tag_count > 0) {
		return NULL;
	}
	return &runtime->tags[index];
}



PLC_Variable_t* get_tag_by_name(Runtime_t* runtime, const char* name) {
	if (runtime == NULL || name == NULL) {
		return NULL;  // Si l'une des valeurs est NULL, on retourne NULL
	}

	for (size_t i = 0; i < runtime->tag_count; i++) {
		if (strcmp(runtime->tags[i].name, name) == 0) {
			return &runtime->tags[i];  // Retourner le tag trouvé
		}
	}

	return NULL;  // Si le tag n'est pas trouvé, on retourne NULL
}


int plc_tag_read(const PLC_Variable_t* tag, void* out_value)
{
	if (!tag || !out_value) return -1;

	if (tag->vtype == PLC_IN || tag->vtype == PLC_OUT) {
		Device_t* dev = tag->io.device;
		BackendDriver_t* drv = dev ? dev->driver : NULL;

		if (!drv || !drv->ops || !drv->ops->get_input_data) return -1;

		uint8_t* base = drv->ops->get_input_data(drv, dev);
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

int plc_tag_write(const PLC_Variable_t* tag, const void* in_value)
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

		if (!drv || !drv->ops || !drv->ops->get_output_data)
			return -1;

		uint8_t* base = (uint8_t*)drv->ops->get_output_data(drv, dev);
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

