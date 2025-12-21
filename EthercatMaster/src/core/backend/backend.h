#pragma once
#include <stdbool.h>

typedef struct BackendDriver BackendDriver_t;
typedef struct BackendConfig BackendConfig_t;

typedef struct {
	int (*init)(BackendDriver_t*);
	int (*start)(BackendDriver_t*);
	int (*process)(BackendDriver_t*);
	int (*stop)(BackendDriver_t*);
	void (*destroy)(BackendDriver_t*);
} BackendDriverOps_t;

struct BackendDriver {
	const char* name;
	BackendDriverOps_t* ops;
	BackendConfig_t* config;
	bool is_initialized;
	int ref_count;
};
