# ---
owner: ODIN
last_reviewed: 2026-05-18
status: draft
doc_id: DOC_INDEX
# ---

# Índice de documentación — LHOS-AEGIS-360

Este índice centraliza los documentos contractuales y su estado básico. Actualizar este archivo cuando se creen, modifiquen o reasignen documentos.

- **README**: [README.md](README.md) — Resumen del repositorio y directrices rápidas.
- **HANDOFF**: [HANDOFF.md](HANDOFF.md) — Estado y criterios de entrega. **Owner:** (por asignar)
- **CHANGELOG**: [CHANGELOG.md](CHANGELOG.md) — Historial de versiones.
- **Architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — Lineamientos arquitectónicos.
- **Protocol Core**: [docs/PROTOCOL_CORE.md](docs/PROTOCOL_CORE.md) — Especificación del parser y FSM.
- **Packet Model**: [docs/PACKET_CENTRIC_MODEL.md](docs/PACKET_CENTRIC_MODEL.md) — Modelo packet-centric y reglas de ownership.
- **ISO12207 Guidelines**: [docs/sprints/ISO12207_GUIDELINES.md](docs/sprints/ISO12207_GUIDELINES.md) — Mapeo documental y requisitos QA.
- **Sprint Matrix**: [docs/sprints/SPRINT_MATRIX.md](docs/sprints/SPRINT_MATRIX.md) — Planeación de sprints.
- **HTTP Monitor API (OpenAPI)**: [docs/sprints/HTTP_MONITOR_OPENAPI.yaml](docs/sprints/HTTP_MONITOR_OPENAPI.yaml) — API de monitorización.
- **Handoff: Packet Ownership**: [docs/handoff/packet_ownership_tracing.md](docs/handoff/packet_ownership_tracing.md) — Trazabilidad de buffers/ownership.
- **Packet Lifecycle**: [docs/PACKET_LIFECYCLE.md](docs/PACKET_LIFECYCLE.md) — Estados y reglas del ciclo de vida de paquetes.
- **Handoff: Packet Ownership**: [docs/handoff/packet_ownership_tracing.md](docs/handoff/packet_ownership_tracing.md) — Trazabilidad de buffers/ownership.
- **QA script**: [QA/check-contract.sh](../QA/check-contract.sh) — Script de chequeo contractual (SPIFFS, PSRAM, heap usage).

Notas:
- Añadir **Owner** y **Estado** (draft/reviewed/approved) en cada documento.
- Ejecutar `QA/check-contract.sh` antes de cada PR relevante.
