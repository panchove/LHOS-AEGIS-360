// pool.c – Implementación stub Sprint 01 (heapless buffer pool)
// QA: sin heap ni new/delete, C17 estricto.
#include "pool.h"
#include <string.h>

static pool_buf_t buffers[POOL_BUF_COUNT];
static uint8_t   allocated[POOL_BUF_COUNT];
static size_t    hwm = 0;

int pool_init(void) {
    memset(allocated, 0, sizeof(allocated));
    for (size_t i = 0; i < POOL_BUF_COUNT; ++i) {
        buffers[i].len = 0;
        buffers[i].magic = 0xDEADBEEF;
    }
    hwm = 0;
    return 0;
}

size_t pool_free_count(void) {
    size_t free = 0;
    for (size_t i = 0; i < POOL_BUF_COUNT; ++i) if (!allocated[i]) ++free;
    return free;
}

pool_buf_t* pool_alloc(void) {
    for (size_t i = 0; i < POOL_BUF_COUNT; ++i) {
        if (!allocated[i]) {
            allocated[i] = 1;
            if (POOL_BUF_COUNT - pool_free_count() > hwm) hwm = POOL_BUF_COUNT - pool_free_count();
            buffers[i].len = 0;
            buffers[i].magic = 0xDEADBEEF;
            return &buffers[i];
        }
    }
    return NULL;
}

void pool_release(pool_buf_t* buf) {
    for (size_t i = 0; i < POOL_BUF_COUNT; ++i) {
        if (&buffers[i] == buf) {
            allocated[i] = 0;
            buffers[i].len = 0;
            buffers[i].magic = 0xBADBAD00;
            return;
        }
    }
    // Ignora buffer fuera del pool (QA puede instrumentar)
}

size_t pool_high_water_mark(void) { return hwm; }
size_t pool_total_count(void) { return POOL_BUF_COUNT; }
