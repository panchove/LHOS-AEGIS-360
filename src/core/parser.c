// parser.c – Stub inicial FSM de parser de paquetes (Sprint 01)
// C17, heapless, integración pool y QA métricas
#include "parser.h"
#include <string.h>

static parser_dispatch_cb_t cb = 0;
static pool_buf_t* current = 0;
static size_t pos = 0, err_count = 0, dispatch_count = 0;

int parser_init(parser_dispatch_cb_t _cb) {
    cb = _cb;
    current = 0;
    pos = err_count = dispatch_count = 0;
    return 0;
}

void parser_feed_byte(uint8_t b) {
    // Stub: acepta solo tramas [0xAA, len, ...len bytes..., CRC8]
    if (!current) {
        if (b == 0xAA) {
            current = pool_alloc();
            if (!current) { ++err_count; return; }
            current->len = 0; pos = 0;
            current->data[pos++] = b;
        }
        return;
    }
    current->data[pos++] = b;
    // Trama mínima [0xAA, len, ...data..., CRC8]
    if (pos == 2) return;
    size_t datalen = current->data[1];
    if (pos < datalen + 3) return;

    // Placeholder CRC: solo suma simple
    uint8_t sum = 0; for (size_t i = 0; i < datalen+2; ++i) sum += current->data[i];
    if (sum == b) {
        current->len = datalen + 2;
        if (cb) cb(current);
        ++dispatch_count;
    } else {
        ++err_count;
        pool_release(current);
    }
    current = 0; pos = 0;
}

void parser_feed_buf(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) parser_feed_byte(data[i]);
}
size_t parser_error_count(void){ return err_count; }
size_t parser_dispatch_count(void){ return dispatch_count; }
