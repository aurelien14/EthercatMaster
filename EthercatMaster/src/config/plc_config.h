#pragma once
#include "core/device/device.h"
#include "backend/ethercat/ethercat_config.h"
#include <stdint.h>
/*
typedef enum {
	BACKEND_ETHERCAT,
	BACKEND_MODBUS,
	BACKEND_TCP
} BackendType_t;
*/


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
	BackendConfig_t* backends;
	size_t backend_count;
	DeviceConfig_t* devices;
	size_t device_count;
} PLCSystemConfig_t;



extern PLCSystemConfig_t plc_system_config;