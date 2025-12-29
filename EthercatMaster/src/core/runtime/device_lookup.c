#include "device_lookup.h"
#include <stddef.h>
#include "core/system/memalloc.h"

//#include <stdint.h>"

DeviceLookup_t* create_device_lookup(Device_t *devices, int devices_count)
{
	DeviceLookup_t* device_lut = CALLOC(devices_count, sizeof(DeviceLookup_t));
	if (!device_lut)
		return NULL;

	for (size_t i = 0; i < devices_count; i++) {
		device_lut[i].name = devices[i].name;
		device_lut[i].dev = &devices[i];
	}

	return device_lut;
}




void find_devices_lookup(Device_t** devices)
{
	
}
