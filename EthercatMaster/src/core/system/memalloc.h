#pragma once
#include <stddef.h>

void* ec_malloc(size_t size, const char* file, int line);
void* ec_calloc(size_t count, size_t size, const char* file, int line);
void  ec_free(void* ptr);

void  ec_mem_report(void);

#ifdef EC_MEM_DEBUG
#define MALLOC(sz)			ec_malloc(sz, __FILE__, __LINE__)
#define CALLOC(count, sz)	ec_calloc(count, sz, __FILE__, __LINE__)
#define FREE(p)				ec_free(p)
#else
#include <stdlib.h>
#define MALLOC(sz)			malloc(sz)
#define CALLOC(count, sz)	calloc(count, sz)
#define FREE(p)				free(p)
#endif
