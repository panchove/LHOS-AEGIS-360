#pragma once
#include <stddef.h>
#include "esp_rom_crc.h"
static inline uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    return ~esp_rom_crc16_be((uint16_t)~0xFFFF, data, len);
}
