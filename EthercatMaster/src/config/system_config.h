#pragma once
#include "core/device/device_desc.h"
#include "core/scheduler/task.h"
#include "backend/ethercat/ethercat_config.h"
#include "core/plc/tags_desc.h"
#include <stdint.h>


/*--------------------------------------------------*/
/*			D R I V E R S    C O N F				*/
/*--------------------------------------------------*/
typedef struct BackendConfig {
	ProtocolType_t type;
	const char* name;    // ex: "ec0", "modbus1", etc
	union {
		EtherCAT_config_t ethercat;
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
typedef struct {
	const char* device_name;

	const DeviceDesc_t* device_desc;  // L230, DAIO16, etc
	const char *backend_name;

	union {
		struct {
			uint16_t expected_position;   // EtherCAT position
			const char* ifname;
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

	const char* device_name;   // lien SYMBOLIQUE
	uint16_t offset;
	uint8_t  bit;
} PLC_TagDesc_Config_t;



/*--------------------------------------------------*/
/*			C O N F I G U R A T I O N				*/
/*--------------------------------------------------*/
typedef struct {
	BackendConfig_t* backends;
	size_t backend_count;

	DeviceConfig_t* devices;
	size_t device_count;

	PLC_TagDesc_Config_t* plc_tags_desc;
	size_t plc_tags_count;
} PLCSystemConfig_t;