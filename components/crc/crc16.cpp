// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#include "crc16.hpp"

namespace crc16 {

constexpr uint16_t ccitt_update_constexpr(uint16_t crc, uint8_t byte) {
    crc ^= static_cast<uint16_t>(byte) << 8;
    for (int i = 0; i < 8; ++i) {
        if (crc & 0x8000) crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
        else crc = static_cast<uint16_t>(crc << 1);
    }
    return crc;
}

uint16_t ccitt_update(uint16_t crc, uint8_t byte) {
    return ccitt_update_constexpr(crc, byte);
}

uint16_t ccitt(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) crc = ccitt_update(crc, data[i]);
    return crc;
}

constexpr uint16_t ccitt_constexpr(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) crc = ccitt_update_constexpr(crc, data[i]);
    return crc;
}

} // namespace crc16
