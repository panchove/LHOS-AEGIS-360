---
doc_id: SDP.3.CRC16.001
doc_type: INTERFACE
version: 0.1.0
status: DRAFT
owner: ODIN
last_reviewed: 2026-05-19
---

# CRC16 API

## Scope
Wrapper C++17 para función CRC16-CCITT en ROM del ESP32-S3.

## Requirements
- Calcular CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF)
- Convertir CRC a hex string (4 dígitos)

## Interface Specs
```cpp
namespace crc16 {
    uint16_t ccitt_false(const uint8_t* data, size_t len);
    void to_hex(uint16_t crc, char* out);  // out debe tener 5 bytes
}
```

## Verification
- `ccitt_false("123456789", 9)` → `0x29B1`
- `to_hex(0x29B1)` → `"29B1"`

## Traceability
- FR-002 (CRC16-CCITT)
- Implementación: ROM `esp_rom_crc16_be()`
