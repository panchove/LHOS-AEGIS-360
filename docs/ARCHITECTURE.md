# LHOS-AEGIS-360 — Architecture Baseline (Reliability/Recovery expansion)

---

Regla arquitectónica ODIN/THOR (Sprint 06):

Este sistema es contractual y permanentemente packet-centric. Streams y fragmentos sólo existen dentro de Driver/Parser Session. Todo EventBus, presión, ownership, métricas y watchdog gobiernan únicamente packets completos. Véase docs/PACKET_CENTRIC_MODEL.md

---

## Reliability & Recovery Integration
- Fault domains: CORE, RADIO, TRANSPORT, STORAGE, PROTOCOL, SENSOR, UI
- Matriz de detección, escalado por nivel (L0-L5)
- Boot loop, cooldown y panic fingerprinting
- Safe mode fuerza operación mínima
- Aislamiento de servicio, quarantine, health propagation
- Evento: entrada/salida safe mode, trigger recovery, service crash

---

## Diagrama de Escalado de Fallo
```
[error dominio]→[retry→restart→isolation→degrade→safe→reboot]
```
