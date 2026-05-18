// SPDX-License-Identifier: Proprietary - LHOS-AEGIS-360
// ODIN Architecture Directive: C++17, no exceptions, no RTTI, heapless core
// Owner: ODIN | Last reviewed: 2026-05-18

#pragma once

#include <cstddef>
#include <cstdint>

namespace crc16 {
    uint16_t ccitt(const uint8_t* data, size_t len);
    uint16_t ccitt_update(uint16_t crc, uint8_t byte);
    constexpr uint16_t ccitt_constexpr(const uint8_t* data, size_t len);
}
