#include "app/plc_tasks.h"
#include "backend/ethercat/ethercat.h"
#include "config/config.h"
#include "core/system/memalloc.h"
#include "core/tags/tags.h"
#include "runtime.h"



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
	long index = strtol(backend_name, &end, 10);
	if (end == backend_name || index < 0) {
		return -1;
	}
	return (int)index;
}


//TODO: mettre dans backend.c, mettre ProtocolType_t au lieu de runtime??????
static BackendDriver_t* runtime_create_backend(Runtime_t* runtime, const BackendConfig_t* drv_cfg) {
	int i = runtime_get_backend_index(drv_cfg->name);
	BackendDriver_t* drv = create_backend_driver(drv_cfg, i);
	if (drv == NULL) {
		printf("[RUNTIME] Unable to create backend %s\n", drv_cfg->name);
	}
	return drv;
}


//TODO: mettre dans backend.c, mettre BackendDriver_t* au lieu de runtime???
static BackendDriver_t* runtime_find_backend(Runtime_t* runtime, const char* backend_name) {
	for (size_t i = 0; i < runtime->backend_count; i++) {
		if (strcmp(runtime->backends[i]->system_name, backend_name) == 0) {
			return runtime->backends[i];
		}
	}
	return NULL;
}


static int runtime_create_plc_tag(Runtime_t* runtime, const PLCSystemConfig_t* plc_config) {
	PLC_Tag_t* tags = CALLOC(1, plc_config->plc_tags_count * sizeof(PLC_Tag_t));
	if (tags == NULL) {
		printf("[RUNTIME] Unable to allocate memory for tags\n");
		return -1;
	}
	runtime->tags = tags;
	runtime->tag_count = plc_config->plc_tags_count;

	for (size_t i = 0; i < plc_config->plc_tags_count; i++) {

		const PLC_TagDesc_Config_t* tc = &plc_config->plc_tags_desc[i];
		PLC_Tag_t* tag = &tags[i];
		if (tag == NULL) {
			printf("[RUNTIME] Error in tag number %ul\n", (unsigned int)i);
			continue;
		}

		// champs communs
		tag->name = tc->name;
		tag->dtype = tc->dtype;
		tag->vtype = tc->vtype;

		switch (tc->vtype) {

		case PLC_IN:
		case PLC_OUT: {
			Device_t* dev = NULL;

			for (size_t j = 0; j < runtime->device_count; j++) {
				if(tc->device_addr == runtime->devices[j]->plc_addr) {
					dev = runtime->devices[j];
					break;
				}
			}

			if (!dev) {
				printf("[TAGS] Device at address %d not found for tag '%s'\n", \
					tc->device_addr, tc->name);
				free(tags);
				return 0;
			}

			tag->io.device = dev;
			tag->io.offset = tc->offset;
			tag->io.bit = tc->bit;
			break;
		}

		case PLC_HMI:
			//tag->hmi.index = allocate_hmi_slot(tag);
			break;

		case PLC_PV:
			//tag->internal.index = allocate_internal_var(tag);
			break;
		}
	}
	return 0;
}





Runtime_t *create_runtime(void) {
	Runtime_t* runtime = CALLOC(1, sizeof(Runtime_t));
	if (runtime == NULL) {
		printf("[RUNTIME] Unable to allocate memory for runtime\n");
		return NULL;
	}
	memset(runtime, 0, sizeof(Runtime_t));
	return runtime;
}


int runtime_init(Runtime_t *runtime, const PLCSystemConfig_t* plc_config) {
	if (runtime == NULL || plc_config == NULL) {
		return -1;
	}

	// 1. Initialize backends
	for (int i = 0; i < plc_config->backend_count; i++) {
		const BackendConfig_t* drv_cfg = &plc_config->backends[i];
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
	//TODO: en cours: ajout bind dans driver, à controler
	for (size_t i = 0; i < plc_config->device_count; i++) {
		const DeviceConfig_t* cfg_dev = &plc_config->devices[i];

		Device_t* dev = device_create(cfg_dev);
		if (dev == NULL) {
			printf("[RUNTIME] Impossible de créer le device %s\n", cfg_dev->device_name);
			return -1;
		}



		BackendDriver_t* drv = runtime_find_backend(runtime, cfg_dev->backend_name);
		if (drv == NULL) {
			printf("[RUNTIME] Backend %s introuvable pour le device %s\n", cfg_dev->backend_name, dev->name);
			return -1;
		}

		if (drv->ops->bind) {
			drv->ops->bind(drv, dev, cfg_dev);
		} 
		else {
			printf("[RUNTIME] Protocole non supporté %d pour le device %s\n", drv->desc->protocol, dev->name);
			return -1;
		}

		dev->backend_handle = drv;
		runtime->devices[runtime->device_count++] = dev;
	}


	//3. Finalize backend mappings for Ethercat
	for (size_t i = 0; i < plc_config->device_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		if (drv != NULL && drv->ops->finalize)
			drv->ops->finalize(drv);
	}


	//4. Initialiser iomap
	if (runtime_create_plc_tag(runtime, plc_config)) {
		//TODO: dois je vider les device et backend drivers?
		return -1;
	}


	//5. Init PLC tasks Scheduler
	scheduler_init(&runtime->plc, runtime, SCHEDULER_BASE_CYCLE_US);
	scheduler_add_task(&runtime->plc,
		&(PLC_Task_t){
		.name = "FAST",
			.period_ms = 100,
			.run = plc_task1_run,
			//.context = runtime,
			.init = 0
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

		if (drv->desc->protocol == PROTO_ETHERCAT) {
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
	scheduler_stop(&runtime->plc);

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

	// Libérer les backends en premier car il alloue les buffers dans les devices
	for (size_t i = 0; i < runtime->backend_count; i++) {
		BackendDriver_t* drv = runtime->backends[i];
		if (drv->desc->destroy)
			drv->desc->destroy(drv);
		runtime->backends[i] = NULL;
	}
	runtime->backend_count = 0;


	// Libérer les devices
	for (size_t i = 0; i < runtime->device_count; i++) {
		Device_t* dev = runtime->devices[i];
		if(dev->desc->destroy)
			dev->desc->destroy(dev);
		runtime->devices[i] = NULL;
	}
	runtime->device_count = 0;

	

	// Libérer les tags
	FREE(runtime->tags);
	runtime->tags = NULL;
	runtime->tag_count = 0;

	//Libérer
	FREE(runtime);
}


void runtime_sync_backends(Runtime_t* rt)
{
	for (size_t i = 0; i < rt->backend_count; i++) {
		BackendDriver_t* drv = rt->backends[i];
		if (drv->ops->sync)
			drv->ops->sync(drv);
	}
}
