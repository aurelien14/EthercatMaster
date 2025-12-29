#pragma once
#include <osal.h>

typedef enum {
	PLC_BOOL,
	PLC_INT,
	PLC_DINT,
	PLC_REAL,
	PLC_STRING,
	PLC_ARRAY
} PLC_DataType_t;

typedef enum {
	PLC_IN,
	PLC_OUT,
	PLC_HMI,
	PLC_PV
} PLC_VarType_t;

#define SIZE_OF_BOOL	1
#define SIZE_OF_INT		4
#define SIZE_OF_DINT	4
#define SIZE_OF_REAL	4

typedef struct {
	const char* name;
	PLC_DataType_t dtype;
	PLC_VarType_t vtype;

	union {
		struct {
			uint16_t offset;
			uint8_t  bit;
		} io;

		struct {
			uint16_t runtime_index;
		} internal;

		struct {
			uint16_t hmi_index;
		} hmi;
	};
} PLC_TagDesc_t;

