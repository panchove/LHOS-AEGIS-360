#pragma once
#include <cstdint>
#include <cstddef>
#include "esp_rom_crc.h"

namespace crc16 {
    // CRC-16/CCITT-FALSE using ROM function wrapper.
    // poly=0x1021, init=0xFFFF
    inline uint16_t ccitt_false(const uint8_t* data, size_t len) {
        if (!data || len == 0) return 0xFFFF;
        // esp_rom_crc16_be expects initial crc value
        // Provide ~0xFFFF to emulate behavior and invert at end
        return ~esp_rom_crc16_be(static_cast<uint16_t>(~0xFFFF), data, len);
    }

    inline void to_hex(uint16_t crc, char* out) {
        const char* hex = "0123456789ABCDEF";
        out[0] = hex[(crc >> 12) & 0x0F];
        out[1] = hex[(crc >> 8) & 0x0F];
        out[2] = hex[(crc >> 4) & 0x0F];
        out[3] = hex[crc & 0x0F];
        out[4] = '\0';
    }
}
