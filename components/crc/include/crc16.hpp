#pragma once
#include <cstddef>
#include <cstdint>

namespace aegis {
namespace crc16 {

constexpr uint16_t ccitt_update_constexpr(uint16_t crc, uint8_t data) {
    crc ^= (uint16_t)data << 8;
    for (int i = 0; i < 8; ++i) {
        if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
        else crc <<= 1;
    }
    return crc;
}

constexpr uint16_t ccitt_constexpr(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) crc = ccitt_update_constexpr(crc, data[i]);
    return crc;
}

uint16_t ccitt(const uint8_t* data, size_t len);

} // namespace crc16
} // namespace aegis
