#pragma once

#include <stddef.h>
#include <stdint.h>

/* Protocoles supportés */
typedef enum {
	PROTO_NONE = 0,
	PROTO_ETHERCAT,
	PROTO_MODBUS_RTU,
	PROTO_MODBUS_TCP,
	PROTO_TCP_CUSTOM
} ProtocolType_t;

/* Description d’un device pour UN protocole donné */
typedef struct {
	ProtocolType_t protocol;
	const void* protocol_desc;
} DeviceProtocolDesc_t;



