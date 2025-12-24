#pragma once
#include <stddef.h>
#include "core/device/device.h"

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
	const char name[8];
	ProtocolType_t protocol;

	const BackendDriverOps_t* ops;

	int is_initialized;
	int ref_count;
};


BackendDriver_t* create_backend_by_proto[PROTO_COUNT];