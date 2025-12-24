#pragma once
typedef enum {
	TAG_BOOL,
	TAG_INT,
	TAG_REAL
} TagType_t;

typedef struct {
	const char* name;
	TagType_t type;
	void* addr;   // pointeur direct vers iomap ou variable
} Tag_t;