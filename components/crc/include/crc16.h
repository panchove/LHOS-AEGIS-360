#pragma once
#include <stddef.h>
#include <stdint.h>

// CRC16-CCITT (0x1021) implementation
uint16_t crc16_ccitt(const uint8_t *data, size_t len);
