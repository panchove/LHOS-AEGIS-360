// test_message_bus.c – QA message bus contract (Sprint 01, ODIN)
#include "message_bus.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int dispatch_called = 0;
void my_bus_cb(pool_buf_t* buf) {
    ++dispatch_called;
    assert(buf && buf->magic == 0xDEADBEEF);
    assert(buf->len == 3);
    assert(buf->data[0] == 0xAA);
    pool_release(buf);
}

void test_pubsub() {
    pool_init(); message_bus_init(); dispatch_called = 0;
    message_bus_subscribe(my_bus_cb);
    pool_buf_t* b = pool_alloc();
    b->data[0]=0xAA; b->data[1]=0xBB; b->data[2]=0xCC; b->len=3;
    int r = message_bus_publish(b);
    assert(r == 0);
    assert(dispatch_called == 1);
    assert(message_bus_publish_count() == 1);
    assert(message_bus_dispatch_count() == 1);
}
void test_no_subscriber() {
    pool_init(); message_bus_init();
    pool_buf_t* b = pool_alloc();
    b->data[0]=0x10; b->len=1;
    // No hay suscriptor: debería hacer release automáticamente
    int r = message_bus_publish(b);
    assert(r == 0);
    assert(pool_free_count() == pool_total_count());
}
int main() {
    printf("[QA] message bus: test_pubsub\n");
    test_pubsub();
    printf("[QA] message bus: test_no_subscriber\n");
    test_no_subscriber();
    puts("[OK] message bus QA tests PASSED");
    return 0;
}
