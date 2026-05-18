# Feature – CRC16 Reference (Sprint 01 — V1)

## 1. Descripción breve
Implementación pura C17 del contrato CRC16-CCITT para framing y verificación QA en todos los flujos core/serial.

## 2. Justificación y contexto contractual
- Contract de APIS_CORE.md sección 2
- Parte del sprint 01 core
- QA: Todos los flows dependen de CRC. Test vector obligatorio.

## 3. Requisitos funcionales
- [ ] API C seventeen: uint16_t crc16_ccitt(const uint8_t*, size_t)
- [ ] Sin dependencias externas

## 4. No funcionales
- [ ] Ejecutable en cualquier plataforma CI

## 5. Checklist entregables
- [ ] crc16.h/.c, tests, vector legacy y moderno

## 6. Acceptance/QA
- [ ] Match con vectors conocidos (legados y estándar)

---

ID contrato: APIS_CORE.md sección 2