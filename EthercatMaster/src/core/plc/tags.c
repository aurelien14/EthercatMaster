#include "config/system_config.h"
#include "core/device/device.h"
#include "tags.h"


PLC_Tag_t* create_plc_tags(size_t count) {
	PLC_Tag_t* t = calloc(1, count * sizeof(PLC_Tag_t));
	if (t == NULL) {
		printf("Error to create iomap tags\n");
	}
	return t;
}

void map_plc_tags(PLC_Tag_t *t, )