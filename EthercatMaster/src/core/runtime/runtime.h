#pragma once
#include "config/system_config.h"
#include "core/device/device.h"
#include "core/backend/backend.h"
#include "core/scheduler/scheduler.h"
#include "core/tags/tags.h"


#if 0
typedef struct {
	BackendDriver_t* backends[MAX_BACKENDS];
	int backend_count;
} Backends_t;

typedef struct {
	Device_t* devices[MAX_DEVICES];
	int device_count;
} Devices_t;

typedef struct {
	Backends_t backends;
	Devices_t devices;
} Runtime_t;
#endif

typedef struct Runtime {
	BackendDriver_t* backends[MAX_BACKENDS];
	int backend_count;

	Device_t* devices[MAX_DEVICES];
	int device_count;

	Scheduler_t plc;
	long system_is_running;


	PLC_Variable_t* tags;
	size_t     tag_count;

} Runtime_t;



Runtime_t* create_runtime(void);

int runtime_init(Runtime_t *runtime, const PLCSystemConfig_t *plc_config);
void runtime_start(Runtime_t* runtime);
void runtime_process(Runtime_t*);
void runtime_stop(Runtime_t*);
void runtime_cleanup(Runtime_t*);
void runtime_sync_backends(Runtime_t* rt);