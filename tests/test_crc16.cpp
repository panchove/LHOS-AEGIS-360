#include "unity.h"
#include "crc16.hpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

TEST_CASE("CRC16-CCITT known vector", "[crc16]") {
    const uint8_t data[] = "123456789";
    uint16_t crc = crc16::ccitt_false(data, 9);
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc);
}

TEST_CASE("CRC16 to hex", "[crc16]") {
    char hex[5];
    crc16::to_hex(0x29B1, hex);
    TEST_ASSERT_EQUAL_STRING("29B1", hex);
}

TEST_CASE("CRC16 empty data", "[crc16]") {
    uint16_t crc = crc16::ccitt_false(nullptr, 0);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc);
}
