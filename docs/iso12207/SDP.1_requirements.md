---
doc_id: SDP.1.REQ.001
doc_type: REQUIREMENTS
version: 0.1.0
status: DRAFT
owner: ODIN
last_reviewed: 2026-05-19
approval_date: TBD
iso_process: SDP.1 (Requirements Elicitation)
traceability: []
---

# LHOS-AEGIS-360 - System Requirements

## 1. Scope

Este documento define los requisitos funcionales y no funcionales del sistema LHOS-AEGIS-360.

### 1.1 In Scope
- Sistema de telemetría sobre ESP32-S3
- Comunicación LTE vía SIMCom A7670
- Persistencia en LittleFS
- Protocolo Layrz (compatible con legacy)
---
doc_id: SDP.1.REQ.001
doc_type: REQUIREMENTS
version: 0.1.0
status: DRAFT
owner: ODIN
last_reviewed: 2026-05-19
---

# System Requirements

## FR-001: Packet Framing
Parseo de `<Px>data</Px>;CRC`

## FR-002: CRC16-CCITT
0x1021, init 0xFFFF, check "123456789" → 0x29B1

## FR-003: Packet Pool
16x512 bytes en PSRAM, fallback DRAM

## FR-004: Message Bus
Colas RT/RI, ownership transfer

## FR-005: BlackBox (LittleFS)
Persistencia, replay

## NFR-001: Heapless core
No malloc/new/delete en hot-paths
- **Verification:** Test de parser con vectores conocidos
