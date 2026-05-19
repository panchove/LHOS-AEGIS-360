# ---
owner: PM
last_reviewed: 2026-05-19
status: approved
doc_id: HANDOFF
# ---

# HANDOFF – LHOS-AEGIS-360

**Fecha:** 2026-05-19
**Arquitecto asignado:** THOR (Developer IA)
**Supervisado por:** ODIN (Arquitecto saliente)

---

## Estado actual del repositorio

El repositorio está en **estado limpio** (freeze). Solo contiene:

- `docs/` - Documentación ISO/IEC 12207
- `legacy_reference/` - Código legacy (solo referencia, no usar)
- `.github/` - Configuración de GitHub (copilot-instructions.md)
- `.vscode/` - Configuración local de VS Code

**No hay código fuente en el repositorio.** Todo el desarrollo se realiza en workspaces locales.

---

## Reglas vinculantes (no negociables)

| Regla | Descripción |
|-------|-------------|
| **C++17** | Sin exceptions, sin RTTI, heapless core |
| **RAII** | Todo recurso con ownership automático |
| **No eFuse** | 100% recoverable por software |
| **No SPIFFS** | Solo LittleFS para persistencia |
| **Anti-zombie** | `rm sdkconfig` antes de cada cambio en defaults |
| **No CI/CD** | Desarrollo sin GitHub Actions |
| **No PRs** | Commits directos a main |

---

## Hardware objetivo

- **Placa:** ESP32-S3-WROOM-1-N8R8
- **Flash:** 8MB
- **PSRAM:** 8MB (N8R8)
- **LTE Modem:** SIMCom A7670 (UART)
- **Puerto:** /dev/ttyUSB0

---

## Problema conocido (sin resolver)

**PSRAM no inicializa** en el hardware actual.

```
E (170) quad_psram: PSRAM chip is not connected, or wrong PSRAM line mode
```

**Diagnóstico:** Pendiente de verificación con test oficial de Espressif.

**Workaround temporal:** Deshabilitar PSRAM en `sdkconfig.defaults` usando `# CONFIG_ESP32S3_SPIRAM_SUPPORT is not set`

---

## Próximos sprints (después del desbloqueo)

| Sprint | Objetivo | Tiempo |
|--------|----------|--------|
| 01 | Core Heapless (Pool, CRC16, Parser) | 4h |
| 02 | MessageBus + BlackBox | 3h |
| 03 | LTE Modem Driver | 3h |
| 04 | Layrz Protocol | 2h |
| 05 | BLE Scanner | 3h |
| 06 | Modbus + RS485 | 2h |
| 07 | Integration + Hardening | 2h |

**Total estimado:** 19 horas (~3 días)

---

## Contacto

- **PM:** [Nombre]
- **Documentación:** `docs/iso12207/`
- **Scripts:** `scripts/build.sh`, `scripts/flash.sh`, `scripts/monitor.sh`

---

**THOR, al recibir este HANDOFF, debes:**

1. Confirmar recepción
2. Esperar autorización del PM para salir del freeze
3. No modificar el repositorio hasta que se autoricen los artefactos de auditoría
