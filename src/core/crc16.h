// crc16.h – CRC16-CCITT (Sprint 01, ODIN)
// Contrato: cero dependencias externas, portable, test vectors históricos
#ifndef CRC16_H
#define CRC16_H
#include <stddef.h>
#include <stdint.h>

// Calcula CRC16-CCITT (0x1021, poly, init=0xFFFF, no reflect, no xorout)
uint16_t crc16_ccitt(const uint8_t* data, size_t len);

#endif // CRC16_H
