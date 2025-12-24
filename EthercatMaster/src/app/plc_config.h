#pragma once
#include "core/device/device_desc.h"
#include "core/scheduler/task.h"
#include "backend/ethercat/ethercat_config.h"
#include "core/plc/tags.h"
#include <stdint.h>



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



typedef struct {
	PLC_TaskFunc plc_task;
	const char* name;
	uint32_t period_us;
} PLC_TaskConfig_t;



typedef struct {
	BackendConfig_t* backends;
	size_t backend_count;
	DeviceConfig_t* devices;
	size_t device_count;
} PLCSystemConfig_t;


typedef struct {
	const char* name;

	//.name = "X15_Out0"
	//type = TAG_BOOL,
	//addr = &((L230_RX_PDO_t*)dev->rx_iomap)->L230_DO_Byte0_bits.X15_Out0
	//}
}PLCTagsConf_t;

extern PLCSystemConfig_t plc_system_config;
