// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

extern "C" {
#include "unity.h"
}

#include "components/parser/include/parser.hpp"
#include "components/pool/include/pool.hpp"
#include <cstring>

static bool callback_invoked = false;
static size_t last_len = 0;

extern "C" void parser_cb(pool_buf_t* b) {
    callback_invoked = true;
    last_len = b->len;
    PacketPool::instance().release(b);
}

extern "C" void test_parser_valid_packet(void) {
    static uint8_t fake_psram[8 * 512];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 8, 512);
    PacketParser p;
    p.init(parser_cb);
    const char* pkt = "HELLO;";
    callback_invoked = false;
    p.feed_bytes(reinterpret_cast<const uint8_t*>(pkt), strlen(pkt));
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL(5, last_len);
}

extern "C" void test_parser_fragmented_packet(void) {
    static uint8_t fake_psram[8 * 256];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 8, 256);
    PacketParser p;
    p.init(parser_cb);
    const char* part1 = "HE";
    const char* part2 = "LL";
    const char* part3 = "O;";
    callback_invoked = false;
    p.feed_bytes(reinterpret_cast<const uint8_t*>(part1), strlen(part1));
    p.feed_bytes(reinterpret_cast<const uint8_t*>(part2), strlen(part2));
    p.feed_bytes(reinterpret_cast<const uint8_t*>(part3), strlen(part3));
    TEST_ASSERT_TRUE(callback_invoked);
    TEST_ASSERT_EQUAL(5, last_len);
}

extern "C" void test_parser_reset_clears_state(void) {
    static uint8_t fake_psram[8 * 128];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 8, 128);
    PacketParser p;
    p.init(parser_cb);
    const char* pkt = "ABC";
    p.feed_bytes(reinterpret_cast<const uint8_t*>(pkt), strlen(pkt));
    p.reset();
    // send terminator only - should be treated as empty -> CRC fail increment
    size_t before = p.get_crc_failures();
    p.feed_byte(';');
    TEST_ASSERT_TRUE(p.get_crc_failures() >= before + 1);
}

extern "C" void test_parser_timeout(void) {
    static uint8_t fake_psram[8 * 128];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 2, 128);
    PacketParser p;
    p.init(parser_cb);
    // Force buffer overflow by sending many bytes
    for (size_t i = 0; i < 2000; ++i) p.feed_byte('x');
    TEST_ASSERT_TRUE(p.get_timeouts() > 0 || p.get_crc_failures() > 0);
}
