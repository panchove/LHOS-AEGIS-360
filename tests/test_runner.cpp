// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// Test runner for Unity-based tests

extern "C" {
#include "unity.h"
}

extern "C" void test_pool_init_with_external_psram(void);
extern "C" void test_pool_alloc_release_cycle(void);
extern "C" void test_pool_exhaustion_returns_nullptr(void);
extern "C" void test_pool_double_release_detection_debug(void);
extern "C" void test_pool_stats_accuracy(void);

extern "C" void test_crc16_known_vector(void);
extern "C" void test_crc16_constexpr_vs_runtime(void);

extern "C" void test_parser_valid_packet(void);
extern "C" void test_parser_fragmented_packet(void);
extern "C" void test_parser_reset_clears_state(void);
extern "C" void test_parser_timeout(void);

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_pool_init_with_external_psram);
    RUN_TEST(test_pool_alloc_release_cycle);
    RUN_TEST(test_pool_exhaustion_returns_nullptr);
    RUN_TEST(test_pool_double_release_detection_debug);
    RUN_TEST(test_pool_stats_accuracy);

    RUN_TEST(test_crc16_known_vector);
    RUN_TEST(test_crc16_constexpr_vs_runtime);

    RUN_TEST(test_parser_valid_packet);
    RUN_TEST(test_parser_fragmented_packet);
    RUN_TEST(test_parser_reset_clears_state);
    RUN_TEST(test_parser_timeout);

    UNITY_END();
}
