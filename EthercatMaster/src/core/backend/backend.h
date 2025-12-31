#pragma once
#include <stddef.h>
#include "core/device/device_desc.h"
#include "backend_desc.h"
#include "core/plateform/plateform.h"

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
	void (*sync)(BackendDriver_t* d); //synchronise double buffering
	int (*stop)(BackendDriver_t*);
	uint8_t* (*get_input_ptr)(BackendDriver_t*, Device_t*);
	uint8_t* (*get_output_ptr)(BackendDriver_t*, Device_t*);
} BackendDriverOps_t;

//sync appelé à la fin d’un cycle PLC
// - publie les sorties
// - prépare le buffer suivant
// - garantit la cohérence temporelle
struct BackendDriver {
	char system_name[8];
	int instance_index;
	const BackendDesc_t* desc;
	const BackendDriverOps_t* ops;

	// Les deux seuls index pour TOUT le bus EtherCAT
	atomic_i32_t active_out_buffer_idx;
	atomic_i32_t active_in_buffer_idx;
	atomic_i32_t rt_out_buffer_idx;
};


BackendDriver_t* create_backend_driver(const BackendConfig_t* config, int instance_index);