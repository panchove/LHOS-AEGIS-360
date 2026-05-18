#pragma once
#include <stddef.h>

// Simple pool API (C17)
typedef struct pool_buf_t {
  void *data;    // pointer to payload (null-terminated optional)
  size_t len;    // length (bytes)
  unsigned id;   // slot id for diagnostics
} pool_buf_t;

// Initialize pool. Returns 0 on success
int pool_init(void);

// Allocate a buffer from the pool. Returns NULL if none available.
pool_buf_t* pool_alloc(void);

// Release buffer back to pool
void pool_release(pool_buf_t* buf);

// Diagnostic: number of free slots
unsigned pool_free_count(void);
