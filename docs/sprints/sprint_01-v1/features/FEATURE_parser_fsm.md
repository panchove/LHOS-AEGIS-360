# Feature – Parser FSM (Sprint 01 — V1)

## 1. Descripción breve
Parser de tramas/paquetes binarios multi-protocolo, validación CRC, FSM con ownership de buffer, integrado con pool heapless.

## 2. Justificación y contexto contractual
- API y restricciones de APIS_CORE.md sección 3
- Es parte del sprint 01 "infraestructura core"
- QA: stats de errores/buffers, contract DFA/crc, ownership.

## 3. Requisitos funcionales
- [ ] Alimento por byte y buffer
- [ ] Callback a consumidor en éxito/CRC ok
- [ ] Ownership, manejo de error, stats QA

## 4. No funcionales
- [ ] Puro C17

## 5. Arquitectura/diseño
- FSM bytestream, integración directa con pool, métricas

## 6. Checklist entregables
- [ ] parser.h/.c, tests, QA
- [ ] Documentation
- [ ] Build (idf.py build) probado en hardware/runner
- [ ] Flash (idf.py -p <puerto> flash) probado y documentado
- [ ] Monitor (idf.py -p <puerto> monitor) ejecutado, logs visibles

## 7. Acceptance criteria / QA
- [ ] Test vector: rejects corrompidos, CRC fail, entrega ok
- [ ] DONE: build, flash, monitor visible en hardware/runner (adjuntar evidencia/log)


---

ID contrato: APIS_CORE.md sección 3