// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

extern "C" {
#include "unity.h"
}

#include "components/pool/include/pool.hpp"

extern "C" void test_pool_init_with_external_psram(void) {
    static uint8_t fake_psram[8 * 512];
    auto& pool = PacketPool::instance();
    TEST_ASSERT_TRUE(pool.init(fake_psram, 8, 512));
}

extern "C" void test_pool_alloc_release_cycle(void) {
    static uint8_t fake_psram[8 * 512];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 4, 128);
    size_t before = pool.free_count();
    pool_buf_t* b = pool.alloc();
    TEST_ASSERT_NOT_NULL(b);
    b->len = 4;
    pool.release(b);
    TEST_ASSERT_EQUAL_SIZE(before, pool.free_count());
}

extern "C" void test_pool_exhaustion_returns_nullptr(void) {
    static uint8_t fake_psram[2 * 64];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 2, 64);
    pool_buf_t* a = pool.alloc();
    pool_buf_t* b = pool.alloc();
    pool_buf_t* c = pool.alloc();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NULL(c);
    pool.release(a);
    pool.release(b);
}

extern "C" void test_pool_double_release_detection_debug(void) {
    static uint8_t fake_psram[4 * 128];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 4, 128);
    pool_buf_t* a = pool.alloc();
    TEST_ASSERT_NOT_NULL(a);
    size_t free_before = pool.free_count();
    pool.release(a);
    // double release should be harmless; free_count should not decrease
    pool.release(a);
    TEST_ASSERT_EQUAL(free_before, pool.free_count());
}

extern "C" void test_pool_stats_accuracy(void) {
    static uint8_t fake_psram[8 * 64];
    auto& pool = PacketPool::instance();
    pool.init(fake_psram, 8, 64);
    size_t total = pool.total_count();
    size_t free0 = pool.free_count();
    TEST_ASSERT_EQUAL(total, free0);
    auto* p = pool.alloc();
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL(total - 1, pool.free_count());
    pool.release(p);
    TEST_ASSERT_EQUAL(total, pool.free_count());
}
