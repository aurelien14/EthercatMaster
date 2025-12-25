#include "config/config.h"
#include "runtime.h"
#include "backend/ethercat/ethercat.h"
#include "backend/ethercat/ethercat_bind.h"
#include "app/plc_tasks.h"
#include <stdlib.h>

static Device_t* runtime_create_device(DeviceDesc_t* device_desc, const char *name) {
	Device_t* dev = calloc(1, sizeof(Device_t));
	if (dev != NULL) {
		dev->desc = device_desc;
		dev->name = name;
	}
	return dev;
}


static int runtime_get_backend_index(const char* backend_name) {
	if(backend_name == NULL) {
		return -1;
	}
	while (*backend_name && !isdigit(*backend_name)) {
		backend_name++;
	}
	if (*backend_name == 0)
		return -1;

	char* end = NULL;
	long index = strtol(backend_name, end, 10);
	if (end == backend_name || index < 0) {
		return -1;
	}
	return (int)index;
}


static BackendDriver_t* runtime_create_backend(Runtime_t* runtime, BackendConfig_t* drv_cfg) {
	if (drv_cfg->type == PROTO_ETHERCAT) {
		int i = runtime_get_backend_index(drv_cfg->name);
		BackendDriver_t* drv = EtherCAT_Driver_Create(&drv_cfg->ethercat, i);
		if (drv == NULL) {
			printf("[RUNTIME] Unable to create backend %s\n", drv_cfg->name);
		}
		return drv;
	}
	else {
		printf("[RUNTIME] Unsupported backend type %d\n", drv_cfg->type);
		return NULL;
	}
}

static BackendDriver_t* runtime_find_backend(Runtime_t* runtime, const char* backend_name) {
	for (size_t i = 0; i < runtime->backend_count; i++) {
		if (strcmp(runtime->backends[i]->name, backend_name) == 0) {
			return runtime->backends[i];
		}
	}
	return NULL;
}


Runtime_t *create_runtime(void) {
	Runtime_t* runtime = (Runtime_t*) malloc(sizeof(Runtime_t));
	if (runtime == NULL) {
		printf("[RUNTIME] Unable to allocate memory for runtime\n");
		return NULL;
	}
	memset(runtime, 0, sizeof(Runtime_t));
	return runtime;
}


int runtime_init(Runtime_t *runtime, PLCSystemConfig_t* plc_config) {
	if (runtime == NULL || plc_config == NULL) {
		return -1;
	}

	// 1. Initialize backends
	for (int i = 0; i < plc_config->backend_count; i++) {
		BackendConfig_t* drv_cfg = &plc_config->backends[i];
		BackendDriver_t* drv = runtime_create_backend(runtime, drv_cfg);
		if(drv) {
			int ret = drv->ops->init(drv);
			if (ret < 0) {
				printf("[RUNTIME] Unable to initialize backend %s\n", drv_cfg->name);
				return -1;
			}
			runtime->backends[runtime->backend_count++] = drv;
		}
	}


	//2. Initialize devices
	for (size_t i = 0; i < plc_config->device_count; i++) {
		Device_t* dev = runtime_create_device(plc_config->devices[i].device_desc, plc_config->devices[i].device_name);
		if (dev == NULL) {
			printf("[RUNTIME] Impossible de créer le device %s\n", plc_config->devices[i].device_name);
			return -1;
		}

		BackendDriver_t* drv = runtime_find_backend(runtime, plc_config->devices[i].backend_name);
		if (drv == NULL) {
			printf("[RUNTIME] Backend %s introuvable pour le device %s\n", plc_config->devices[i].backend_name, dev->name);
			return -1;
		}

		if (drv->protocol == PROTO_ETHERCAT) {
			ethercat_bind_device((EtherCAT_Driver_t*)drv, dev, &plc_config->devices[i]);
		} 
		else {
			printf("[RUNTIME] Protocole non supporté %d pour le device %s\n", drv->protocol, dev->name);
			return -1;
		}

		dev->backend_handle = drv;
		runtime->devices[runtime->device_count++] = dev;

	}


	//3. Finalize backend mappings for Ethercat
	for(size_t i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		if (drv != NULL) {
			if (drv->protocol == PROTO_ETHERCAT) {
				Sleep(1000);
				ethercat_finalize_mapping((EtherCAT_Driver_t*)drv);
			}
		}
		else {
			printf("[RUNTIME] Backend null lors de la finalisation du mapping\n");
			return -1;
		}
	}

	//4. Init PLC tasks Scheduler
	scheduler_init(&runtime->plc, SCHEDULER_BASE_CYCLE_US);
	scheduler_add_task(&runtime->plc,
		&(PLC_Task_t){
			.name = "FAST",
			.period_ms = 1000,
			.run = plc_task1_run,
			.context = runtime
	});

	return 0;
}


void runtime_start(Runtime_t* runtime) {
	for (size_t i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		drv->ops->start(drv);
	}
	scheduler_start(&runtime->plc);
	runtime->system_is_running = 1;
}


void runtime_process(Runtime_t* runtime) {
	if (runtime == NULL)
		return;
	

	for (int i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		if (drv->ops->process)
			drv->ops->process(drv);

		if (drv->protocol == PROTO_ETHERCAT) {
			EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)drv;
			/*printf("[ECAT] cycles=%llu cycle[min/max]=%u/%u us jitter[min/max]=%u/%u us\n",
				d->stats.total_cycles,
				d->stats.min_cycle_time_us,
				d->stats.max_cycle_time_us,
				d->stats.min_jitter_us,
				d->stats.max_jitter_us);*/
			printf("[ECAT] jitter: %u us\n",
				d->stats.jitter_us);
		}
	}



	return;
}


void runtime_stop(Runtime_t* runtime) {
	if (runtime == NULL) {
		return;
	}
	//stopper le scheduler
	//scheduler_stop(&runtime->plc);

	//stopper les backends drivers
	for (size_t i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		drv->ops->stop(drv);
	}
	runtime->system_is_running = 0;
}


void runtime_cleanup(Runtime_t* runtime) {
	if (runtime == NULL) {
		return;
	}
	// Libérer les devices
	for (size_t i = 0; i < runtime->device_count; i++) {
		Device_t* dev = runtime->devices[i];
		free(dev);
	}
	// Libérer les backends
	for (size_t i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		drv->ops->destroy(drv);
	}
	free(runtime);
}