#include "config/config.h"
#include "runtime.h"
#include "backend/ethercat/ethercat.h"
#include <stdlib.h>

//TODO: make it dynamic in a structure and use it as argument in runtime_start
//static BackendDriver_t* backend_drivers[MAX_BACKENDS_DRIVERS];
//static int backend_driver_count = 0;

static size_t runtime_get_device_count(const char* backend_name, DeviceConfig_t* device_cfg, size_t len) {
	size_t count = 0;
	for (int i = 0; i < len; i++) {
		if (strcpy(device_cfg[i].backend_name, backend_name) == 0) {
			count++;
		}
	}
}

static const BackendDriver_t* runtime_get_backend_driver(Runtime_t *runtime, BackendConfig_t* backend_cfg) {
	BackendDriver_t* drv = NULL;

	for (size_t i = 0; i < runtime->backend_count; i++) {
		if (runtime->backends[i]->config == backend_cfg) {
			printf("[RUNTIME] Backend %s already initialized\n", backend_cfg->name);
			return runtime->backends[i];
		}
	}

	switch (backend_cfg->type) {
		case PROTO_ETHERCAT:
			drv = (BackendDriver_t*) EtherCAT_Driver_Create(backend_cfg->ethercat.ifname);
			break;
	}

	if (drv == NULL) {
		printf("[RUNTIME] Unable to allocate memory for backend %s", backend_cfg->name);
		return NULL;
	}

	drv->config = backend_cfg;
	runtime->backends[runtime->backend_count++] = drv;
	return drv;
}


static Device_t* runtime_get_device(DeviceConfig_t* dev_cfg) {
	Device_t* dev = NULL;
	DeviceProtocolDesc_t* desc = (DeviceProtocolDesc_t*)device_find_protocol_desc(dev_cfg->device_desc, PROTO_ETHERCAT);
	if (desc == NULL) {
		printf("[RUNTIME] Protocol description not found for device %s\n", dev_cfg->device_name);
		return NULL;
	}
	if (strcmp(dev_cfg->device_desc->model, "L230") == 0) {
		dev = L230_Create();
		if (dev == NULL) {
			printf("[RUNTIME] unable to allocate memory for device %s", dev_cfg->device_desc->model);
			return -1;
		}
		dev->name = dev_cfg->device_name;
		dev->desc = dev_cfg->device_desc;
		dev->protocol = desc->protocol;
		dev->backend_handle = NULL;
	}

	return dev;
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
	//initialize backends
	for (size_t i = 0; i < plc_config->backend_count; i++) {
		BackendConfig_t* dev_cfg = &plc_config->backends[i];
		size_t dev_count = runtime_get_device_count(dev_cfg->name, plc_config->devices, plc_config->device_count);

		BackendDriver_t *drv = runtime_get_backend_driver(runtime, dev_cfg);
		if (drv == NULL) {
			return -1;
		}

		runtime->backends[runtime->backend_count++] = drv;
		//initialize backend if not already done
		if (!drv->is_initialized) {
			int ret = drv->ops->init(drv);
			if (ret < 0) {
				return ret;
			}
			drv->is_initialized = 1;
		}

		drv->ref_count++;
	}

	//initialize devices
#if 0
	for (size_t i = 0; i < plc_config->device_count; i++) {
		DeviceConfig_t* dev_cfg = &plc_config->devices[i];
		Device_t* dev = runtime_get_device(dev_cfg);
		if (dev == NULL) {
			printf("[RUNTIME] Unable to create device %s\n", dev_cfg->device_name);
			return -1;
		}
		//link device to backend
		for (size_t j = 0; j < backend_driver_count; j++) {
			if (backend_drivers[j]->config->name == dev_cfg->backend_name) {
				dev->backend_handle = (void*) backend_drivers[j];
				break;
			}
		}
		if (dev->backend_handle == NULL) {
			printf("[RUNTIME] Unable to find backend %s for device %s\n", dev_cfg->backend_name, dev_cfg->device_name);
			free(dev);
			return -1;
		}

		printf("[RUNTIME] Device %s initialized\n", dev_cfg->device_name);
	}

#endif
}


void runtime_start(Runtime_t* runtime) {
	for (size_t i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		drv->ops->start(drv);
	}
}