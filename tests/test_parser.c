// test_parser.c – QA para FSM parser (Sprint 01, ODIN)
#include "src/core/parser.h"
#include <assert.h>
#include <stdio.h>

static int g_dispatch = 0;
void mydispatch(pool_buf_t* buf) {
    ++g_dispatch;
    assert(buf && buf->len > 0);
    pool_release(buf);
}

void test_parser_simple() {
    assert(parser_init(mydispatch) == 0);
    pool_init();
    g_dispatch = 0;
    // Trama válida: [0xAA, 0x02, 0x12, 0x34, checksum]
    uint8_t frame[] = {0xAA, 0x02, 0x12, 0x34, 0xAA+0x02+0x12+0x34};
    parser_feed_buf(frame, sizeof(frame));
    assert(g_dispatch == 1);
    assert(parser_error_count() == 0);
    assert(parser_dispatch_count() == 1);
}

void test_parser_crc_fail() {
    parser_init(mydispatch); pool_init(); g_dispatch = 0;
    uint8_t frame[] = {0xAA, 0x02, 0x12, 0x00, 0x01}; // crc fake
    parser_feed_buf(frame, sizeof(frame));
    assert(g_dispatch == 0);
    assert(parser_error_count() > 0);
}

void test_parser_partial() {
    parser_init(mydispatch); pool_init(); g_dispatch = 0;
    // Trama incompleta nunca despacha
    uint8_t frame[] = {0xAA, 0x02, 0xC0};
    parser_feed_buf(frame, sizeof(frame));
    assert(g_dispatch == 0);
    assert(parser_error_count() == 0);
}

int main() {
    puts("[QA] parser: test_parser_simple");
    test_parser_simple();
    puts("[QA] parser: test_parser_crc_fail");
    test_parser_crc_fail();
    puts("[QA] parser: test_parser_partial");
    test_parser_partial();
    puts("[OK] parser FSM QA tests PASSED");
    return 0;
}
