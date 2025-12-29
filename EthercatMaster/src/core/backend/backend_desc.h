#pragma once
#include "config/protocol.h"


typedef struct BackendDriver BackendDriver_t;
typedef struct BackendConfig BackendConfig_t;


typedef struct BackendDesc {
	const char* name;
	ProtocolType_t protocol;

	BackendDriver_t* (*create)(BackendConfig_t* config, int instance_index);
	void (*destroy)(BackendDriver_t*);
} BackendDesc_t;