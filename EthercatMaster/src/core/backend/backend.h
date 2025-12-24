#pragma once
#include <stddef.h>
#include "core/device/device_desc.h"

#define MAX_BACKENDS_DRIVERS  3
#define MAX_BACKEND_DEVICES 32

typedef struct BackendDriver BackendDriver_t;

typedef struct {
	int (*init)(BackendDriver_t*);
	int (*start)(BackendDriver_t*);
	int (*process)(BackendDriver_t*);
	int (*stop)(BackendDriver_t*);
	void (*destroy)(BackendDriver_t*);
} BackendDriverOps_t;


struct BackendDriver {
	char name[8];
	ProtocolType_t protocol;

	const BackendDriverOps_t* ops;

	/*int is_initialized;
	int ref_count;*/
};