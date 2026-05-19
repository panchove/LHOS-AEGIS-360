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

### 1.2 Out of Scope
- Interfaz de usuario gráfica
- Web server embebido
- Bluetooth Classic (solo BLE)

## 2. Normative References

| Documento | ID |
|-----------|-----|
| ISO/IEC 12207 | Procesos ciclo de vida |
| ESP-IDF v5.5.3 | Framework oficial |
| SIMCom A7670 Hardware Design | HW Design v1.00 |

## 3. Terms and Definitions

| Término | Definición |
|---------|-------------|
| Pool | Buffer estático en PSRAM para paquetes |
| Heapless | Sin asignaciones dinámicas en runtime |
| Packet-centric | Todo el sistema opera sobre paquetes completos |
| BlackBox | Persistencia de mensajes en LittleFS |

## 4. Functional Requirements

### FR-001: Packet Framing
- **Description:** El sistema debe recibir/transmitir paquetes con formato `<PX>data</PX>;CRC`
- **Priority:** MUST
- **Verification:** Test de parser con vectores conocidos

### FR-002: CRC16 Validation
- **Description:** Cada paquete debe incluir CRC16-CCITT al final
- **Priority:** MUST
- **Verification:** Test vector "123456789" → 0x29B1

### FR-003: Packet Pool
- **Description:** Pool de buffers en PSRAM (fallback a DRAM)
- **Priority:** MUST
- **Verification:** alloc/release cycle test

### FR-004: Message Bus
- **Description:** Colas RT y RI con ownership transfer
- **Priority:** MUST
- **Verification:** publish/consume sin memory leaks

### FR-005: BlackBox (LittleFS)
- **Description:** Persistencia de mensajes cuando no hay red
- **Priority:** MUST
- **Verification:** Replay after power cycle

### FR-006: Layrz Commands
- **Description:** Comandos remotos vía TCP socket
- **Priority:** MUST
- **Verification:** reboot, factory_reset, get_config

### FR-007: LTE Modem (SIMCom A7670)
- **Description:** Inicialización, TCP socket, AT commands
- **Priority:** MUST
- **Verification:** Conectar a red, establecer TCP

### FR-008: BLE Scanner
- **Description:** Escaneo BLE y decodificación de dispositivos conocidos
- **Priority:** SHOULD
- **Verification:** Publicar `<Pb>` para ELA_*, LYWSD03MMC

### FR-009: GNSS (opcional)
- **Description:** Parsing de NMEA desde UART
- **Priority:** COULD
- **Verification:** GGA, RMC sentences

## 5. Non-Functional Requirements

### NFR-001: Heapless Core
- **Description:** Prohibido malloc/new/delete en hot-paths
- **Verification:** check-contract.sh

### NFR-002: Memory Pool
- **Description:** 16 buffers de 512 bytes en PSRAM
- **Verification:** pool_free_count() después de ciclo

### NFR-003: CRC Performance
- **Description:** < 10µs por paquete de 512 bytes
- **Verification:** esp_timer_get_time()

### NFR-004: Boot Time
- **Description:** < 5 segundos a estado READY
- **Verification:** Log timestamps

### NFR-005: Watchdog
- **Description:** Task watchdog habilitado (5 segundos)
- **Verification:** Simular stall, verificar reboot

## 6. Verification Criteria

| Requisito | Método | Criterio |
|-----------|--------|----------|
| FR-001 | Test unitario | Parser pasa vectores |
| FR-002 | Test unitario | CRC known vector pasa |
| FR-003 | Test unitario | alloc/release 1000 ciclos sin leak |
| FR-004 | Test integración | Publish/consume 1000 msgs |
| FR-005 | Test hardware | Replay después de reset |
| FR-006 | Test integración | Comando reboot funciona |
| NFR-001 | Script automático | check-contract.sh pasa |
| NFR-002 | Runtime test | pool_free_count() = total |

## 7. Traceability

| Requisito | Origen | Documento relacionado |
|-----------|--------|----------------------|
| FR-001 | legacy_reference | PROTOCOL_FRAMING.md |
| FR-002 | legacy_reference | CRC16 spec |
| FR-003 | Architectural decision | MEMORY_POLICY.md |
| FR-004 | Architectural decision | MESSAGE_BUS_API.md |
| FR-005 | Architectural decision | BLACKBOX_SPEC.md |
| FR-006 | legacy_reference | LAYRT_COMMANDS.md |

## 8. Revision History

| Version | Fecha | Autor | Cambios |
|---------|-------|-------|---------|
| 0.1.0 | 2026-05-19 | ODIN | Creación inicial |
