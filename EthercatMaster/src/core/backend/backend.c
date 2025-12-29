#include "backend.h"
#include "config/system_config.h"

//TODO: mettre dans runtime : runtime_create_backend_driver 
BackendDriver_t* create_backend_driver(const BackendConfig_t* config, int instance_index)
{
	BackendDriver_t* dev = config->driver_desc->create(config, instance_index);
	if (dev != NULL) {
		dev->instance_index = instance_index; 
		dev->desc = config->driver_desc;
	}
	
	return dev;
}

