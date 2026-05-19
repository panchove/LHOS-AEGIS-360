---
doc_id: SDP.3.PARSER.001
doc_type: INTERFACE
version: 0.1.0
status: DRAFT
owner: ODIN
last_reviewed: 2026-05-19
---

# PacketParser API

## Scope
FSM para parseo de paquetes con formato `<Px>data</Px>;CRC`.

## Requirements
- Detectar tags `<Pd>`, `<Pb>`, `<Pc>`, etc.
- Validar CRC16 al final del paquete
- Timeout por paquete (500ms default)
- Soporte para fragmentación (múltiples `feed_byte`)

## Interface Specs
```cpp
class PacketParser {
public:
    bool init(parser_dispatch_cb_t cb, uint32_t timeout_ms = 500);
    void feed_byte(uint8_t b);
    void feed_bytes(const uint8_t* data, size_t len);
    void reset();
    void check_timeout();  // Llamar periódicamente
    size_t get_packets_ok() const;
    size_t get_crc_failures() const;
};
```

## Verification
- `<Pd>test</Pd>29B1` → callback llamado
- CRC inválido → crc_failures incrementa
- Timeout sin cierre de tag → timeout incrementa

## Traceability
- FR-001 (Packet Framing)
- FR-002 (CRC16-CCITT)
