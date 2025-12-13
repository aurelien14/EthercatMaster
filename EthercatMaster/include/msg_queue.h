#pragma once

#include <stdatomic.h>

#define CMD_QUEUE_CAPACITY 1024 // power-of-two recommended
#define CMD_TEXT_LEN 128


typedef enum {
	CMD_DEBUG,
	CMD_PRINT,
	CMD_SET_PARAM,
	CMD_TRIGGER_ACTION,
	CMD_NOOP
} CommandType;


typedef struct {
	CommandType type;
	uint32_t param_id;
	int32_t value;
	char text[CMD_TEXT_LEN];
} CommandMsg;


typedef struct {
	CommandMsg buffer[CMD_QUEUE_CAPACITY];
	atomic_uint32_t head; // next write index
	atomic_uint32_t tail; // next read index
	// stats
	atomic_uint32_t dropped_count;
	uint32_t capacity; // convenience
} CommandQueue;


static CommandQueue cmdq;