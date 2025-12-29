#include "memalloc.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	void* ptr;
	size_t size;
	const char* file;
	int line;
} alloc_t;

#define MAX_ALLOCS 256

static alloc_t allocs[MAX_ALLOCS];
static int alloc_count = 0;

void* ec_malloc(size_t size, const char* file, int line)
{
	void* p = malloc(size);
	if (!p)
		return NULL;

	for (int i = 0; i < MAX_ALLOCS; i++) {
		if (!allocs[i].ptr) {
			allocs[i] = (alloc_t){ p, size, file, line };
			alloc_count++;
			break;
		}
	}
	return p;
}

void* ec_calloc(size_t count, size_t size, const char* file, int line)
{
	void* p = calloc(count, size);
	if (!p)
		return NULL;

	for (int i = 0; i < MAX_ALLOCS; i++) {
		if (!allocs[i].ptr) {
			allocs[i] = (alloc_t){ p, size, file, line };
			alloc_count++;
			break;
		}
	}
	return p;
}

void ec_free(void* ptr)
{
	if (!ptr)
		return;

	for (int i = 0; i < MAX_ALLOCS; i++) {
		if (allocs[i].ptr == ptr) {
			allocs[i].ptr = NULL;
			alloc_count--;
			break;
		}
	}
	free(ptr);
}

void ec_mem_report(void)
{
	if (alloc_count == 0) {
		printf("[MEM] No leaks detected\n");
		return;
	}

	printf("[MEM] %d leaks detected:\n", alloc_count);
	for (int i = 0; i < MAX_ALLOCS; i++) {
		if (allocs[i].ptr) {
			printf(" - %p (%zu bytes) at %s:%d\n",
				allocs[i].ptr,
				allocs[i].size,
				allocs[i].file,
				allocs[i].line);
		}
	}
}
