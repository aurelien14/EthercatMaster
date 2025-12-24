#pragma once
typedef enum {
	PROTO_ETHERCAT = 0,
	/*PROTO_MODBUS,
	PROTO_TCP,
	PROTO_MODBUS_RTU,
	PROTO_TCP_CUSTOM,*/
	PROTO_COUNT
} ProtocolType_t;


typedef struct {
	const char* model;
	ProtocolType_t protocol;
	const void* hw_desc;
} DeviceDesc_t;