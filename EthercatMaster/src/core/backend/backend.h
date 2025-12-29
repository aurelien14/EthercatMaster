#pragma once
#include <stddef.h>
#include "core/device/device_desc.h"
#include "backend_desc.h"

//defined in config.h
/*#define MAX_BACKENDS_DRIVERS  3
#define MAX_BACKEND_DEVICES 32
*/

/* Forward declarations */
typedef struct Device Device_t;
typedef struct BackendDriver BackendDriver_t;

typedef struct {
	int (*init)(BackendDriver_t*);
	int (*bind)(BackendDriver_t*, Device_t *dev, const void* backend_cfg);
	int (*finalize)(BackendDriver_t*);
	int (*start)(BackendDriver_t*);
	int (*process)(BackendDriver_t*);
	int (*stop)(BackendDriver_t*);
} BackendDriverOps_t;


struct BackendDriver {
	char system_name[8];
	int instance_index;
	BackendDesc_t* desc;
	const BackendDriverOps_t* ops;
};


BackendDriver_t* create_backend_driver(const BackendConfig_t* config, int instance_index);