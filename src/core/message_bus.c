// message_bus.c – Message bus demo (Sprint 01, ODIN)
#include "message_bus.h"

static message_bus_cb_t sub = 0;
static size_t published = 0, dispatched = 0;

int message_bus_init(void) {
    sub = 0; published = dispatched = 0; return 0;
}

int message_bus_publish(pool_buf_t* buf) {
    ++published;
    if (sub) { sub(buf); ++dispatched; } else pool_release(buf);
    return 0;
}

void message_bus_subscribe(message_bus_cb_t cb) { sub = cb; }

size_t message_bus_publish_count(void){ return published; }
size_t message_bus_dispatch_count(void){ return dispatched; }
