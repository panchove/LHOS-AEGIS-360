// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

extern "C" {
#include "unity.h"
}

#include "components/crc/include/crc16.hpp"

extern "C" void test_crc16_known_vector(void) {
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    uint16_t c = crc16::ccitt(data, 9);
    TEST_ASSERT_EQUAL_HEX16(0x29B1, c);
}

extern "C" void test_crc16_constexpr_vs_runtime(void) {
    const uint8_t data[] = {'t','e','s','t'};
    uint16_t r = crc16::ccitt(data, 4);
    uint16_t c = crc16::ccitt_constexpr(data, 4);
    TEST_ASSERT_EQUAL_HEX16(r, c);
}
