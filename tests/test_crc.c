// test_crc.c – Test unitario y compatibilidad legacy/modern (Sprint 01, ODIN)
#include "crc16_esp.h"
#include <assert.h>
#include <stdio.h>

void test_standard_vector() {
    // CRC16-CCITT (0x1021, init=0xFFFF): "123456789" -> 0x29B1
    static const uint8_t vec[] = { '1','2','3','4','5','6','7','8','9' };
    uint16_t result = crc16_ccitt(vec, 9);
    assert(result == 0x29B1);
}
void test_empty() {
    assert(crc16_ccitt(0,0) == 0xFFFF);
}
void test_legacy_vector() {
    // Ejemplo típico: {0xAA, 0x02, 0x12, 0x34} (vector ficticio)
    static const uint8_t legacy[] = {0xAA,0x02,0x12,0x34};
    uint16_t r = crc16_ccitt(legacy, 4);
    // El valor exacto depende del framing; solo documenta para consistencia
    printf("legacy CRC16(AA 02 12 34): 0x%04X\n", (unsigned)r);
}
int main() {
    printf("[QA] crc16: standard vector\n");
    test_standard_vector();
    printf("[QA] crc16: empty\n");
    test_empty();
    printf("[QA] crc16: legacy vector\n");
    test_legacy_vector();
    puts("[OK] CRC16 QA tests PASSED");
    return 0;
}
