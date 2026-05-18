# HANDOF – Estado y transferencia de LHOS-AEGIS-360

**Fecha:** 2026-05-18

## Estado actual
- Documentación lista, alineada con ISO/IEC 12207.
- Tag de entrega inicial: `v0.1.0-sprint-handoff` (este mismo commit está versionado y auditable en el remoto para handoff formal).

- Políticas de independencia legal y control de licencias del legacy formalizadas.
- Contratos de API, monitorización (WDT, tasks), UI y todas las restricciones técnicas explícitas y versionadas.
- Lista para iniciar fase de desarrollo “from scratch”, sin contaminación de código legacy.

## Acciones próximas sugeridas
14: - Si se integran nuevas APIs externas, **auditar y documentar licencias** ANTES de liberar a producción.
15: - Mantener auditoría e independencia contractual durante todo el desarrollo.
16: - POLÍTICA: Toda persistencia (configuración, logs, settings, estados de servicios, blackbox, etc) debe implementarse estrictamente sobre LittleFS. El uso de SPIFFS está prohibido y QA rechazará cualquier feature/configuración/producto que lo refiera, inicialice o habilite incluso parcialmente. Pre-entrega: Ejecutar el script `QA/check-contract.sh` y bloquear merge/QA si detecta referencia indebida a SPIFFS en código, features o tests activos. Cualquier excepción debe contar con aprobación escrita de Arquitectura y un plan de migración prioritaria a LittleFS.
17: 
## Contacto / responsable
- [Nombre encargado, email o Slack según organización]

---

REGLA: Antes de cualquier compactación, milestone, refactor o merge importante, actualizar SIEMPRE HANDOF.md, README.md y CHANGELOG.md con:
- Estado/tags actuales
- Cambios clave
- Contratos/features/documentos afectados
- Nota/decisión relevante para QA, PO, legales o auditoría
Solo luego proceder a compaction/refactor/tag/merge.

Este archivo documenta el estado inicial, garantías legales y los criterios de entrega/recepción para permitir validación, auditoría y continuidad sin bloqueos técnicos ni legales.
