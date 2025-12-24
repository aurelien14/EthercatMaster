#pragma once
#include <stdint.h>

typedef struct EtherCAT_config {
	const char* ifname;  // ex: "eth0"
	uint32_t cycle_us;
	uint32_t io_map_size;
} EtherCAT_config_t;