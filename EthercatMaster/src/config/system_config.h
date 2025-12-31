#pragma once
#include "config/protocol.h"
#include "core/backend/backend_desc.h"
#include "core/device/device_desc.h"
#include "core/tags/tags_desc.h"
#include "core/scheduler/task.h"
#include <stdbool.h>
#include <stdint.h>
/*--------------------------------------------------*/
/*			D R I V E R S    C O N F				*/
/*--------------------------------------------------*/
typedef struct BackendConfig {
	const char* name;    // ex: "ec0", "modbus1", etc
	const BackendDesc_t* driver_desc;
	union {
		struct {
			const char* ifname; // ex: "eth0" } ethercat;
			uint8_t has_dc_clock;
			uint32_t cycle_us;
			uint32_t io_map_size;
		} ethercat;
		struct {
			const char* ip_address;
			uint16_t port;
		} tcp;
		struct {
			const char* ip_address;
			uint16_t port;
		} modbus;
	};
} BackendConfig_t;

				
/*--------------------------------------------------*/
/*			D E V I C E S    C O N F				*/
/*--------------------------------------------------*/
typedef struct DeviceConfig {
	const char* device_name;
	uint16_t plc_address; //logical plc address
	const DeviceDesc_t* device_desc;  // L230, DAIO16, etc
	const char *backend_name;

	union {
		struct {
			uint16_t expected_position;   // EtherCAT position
		} ethercat;

		struct {
			uint8_t slave_id;
		} modbus;
	};

	void* static_params; // optionnel
} DeviceConfig_t;


/*--------------------------------------------------*/
/*			P L C   T A S K   C O N F				*/
/*--------------------------------------------------*/
//TODO: faire une config statique pour les taches comme pour les drivers
typedef struct {
	PLC_TaskFunc plc_task;
	const char* name;
	uint32_t period_us;
} PLC_TaskConfig_t;



/*--------------------------------------------------*/
/*			P L C   T A G S   C O N F				*/
/*--------------------------------------------------*/
typedef struct {
	const char* name;
	PLC_DataType_t dtype;
	PLC_VarType_t vtype;

	uint16_t device_addr;   // adresse logique PLC
	uint16_t offset;
	uint8_t  bit;
	PLC_TagValue_t initial_value;
} PLC_Variables_Config_t;



/*--------------------------------------------------*/
/*			C O N F I G U R A T I O N				*/
/*--------------------------------------------------*/
typedef struct {
	BackendConfig_t* backends;
	size_t backend_count;

	DeviceConfig_t* devices;
	size_t device_count;

	const PLC_Variables_Config_t* plc_variables;
	size_t plc_variables_count;
} PLCSystemConfig_t;


//#define PLC_ADDRESS_DEV_NONE  -1