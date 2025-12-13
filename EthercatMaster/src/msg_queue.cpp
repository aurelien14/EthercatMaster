#include "msg_queue.h"

bool msg_q_init(CommandQueue* q) {
	return true;
}

bool msg_q_push(CommandQueue* q, const CommandMsg* msg) {
	uint32_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
	uint32_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
	uint32_t next = (head + 1) & (q->capacity - 1);


	if (next == tail) {
		atomic_fetch_add_explicit(&q->dropped_count, 1, memory_order_relaxed);
		return false; // full
	}


	// copy message
	q->buffer[head] = *msg; // memcpy-style, OK si POD


	// publish
	atomic_store_explicit(&q->head, next, memory_order_release);
	return true;
}


bool msg_q_pop(CommandQueue* q, CommandMsg* out) {
	uint32_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
	uint32_t head = atomic_load_explicit(&q->head, memory_order_acquire);


	if (tail == head) return false; // empty


	*out = q->buffer[tail];
	atomic_store_explicit(&q->tail, (tail + 1) & (q->capacity - 1), memory_order_release);
	return true;
}