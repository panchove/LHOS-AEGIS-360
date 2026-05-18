// test_pool.c – Unit test para heapless pool (Sprint 01, QA)
// Ejecutable como test directo
#include "pool.h"
#include <assert.h>
#include <stdio.h>

void test_init_and_alloc_release() {
    assert(pool_init() == 0);
    size_t n = pool_total_count();
    pool_buf_t* bufs[32] = {0};
    size_t i, count = 0;
    for (i = 0; i < n; ++i) {
        bufs[i] = pool_alloc();
        assert(bufs[i] != 0 && bufs[i]->magic == 0xDEADBEEF);
        ++count;
    }
    assert(pool_free_count() == 0);
    assert(pool_alloc() == 0); // debe estar agotado
    for (i = 0; i < count; ++i) pool_release(bufs[i]);
    assert(pool_free_count() == n);
}

void test_double_release_safe() {
    pool_init();
    pool_buf_t* buf = pool_alloc();
    assert(buf);
    pool_release(buf);
    pool_release(buf); // debe ser idempotente y seguro
    assert(pool_free_count() == pool_total_count());
}

int test_pool_main(void) {
    puts("[QA] pool: test_init_and_alloc_release");
    test_init_and_alloc_release();
    puts("[QA] pool: test_double_release_safe");
    test_double_release_safe();
    puts("[OK] pool heapless QA tests PASSED");
    return 0;
}
