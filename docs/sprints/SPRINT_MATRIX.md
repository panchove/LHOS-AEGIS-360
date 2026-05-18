# Matriz de Planeación de Sprints – LHOS-AEGIS-360

> **Política:** Solo 3 estados permitidos por sprint: PENDING, IN-PROGRESS, DONE. El avance es secuencial y ahora se alinea versiones Layrz: primero v1, luego v2, luego v2.5. Un sprint no puede avanzar hasta que el anterior esté DONE.

## Resumen de Sprints (por versión Layrz)

| Sprint        | Versión Layrz    | Estado       | Features Incluidos                                                                |
|---------------|------------------|-------------|----------------------------------------------------------------------------------|
| Sprint 1-v1   | 1                | IN-PROGRESS | Buffers, CRC16, Parser FSM, QA, config mínima versión 1, gating y pruebas v1      |
| Sprint 2-v1   | 1                | PENDING     | Message Bus C API, Ownership, Integration, QA/benchmarks solo modo v1             |
| Sprint 3-v1   | 1                | PENDING     | Servicios/Decoders/API C++17, Auto-discovery, Validación v1, monitoreo básico     |
| Sprint 4-v1   | 1                | PENDING     | Testing extendido, Safe Mode, recovevry, T&V Layrz v1                             |
| Sprint 1-v2   | 2                | PENDING     | Migración base a v2, ajuste parser, gating según "lzv==2", features v2 solo v2   |
| Sprint 2-v2   | 2                | PENDING     | Message Bus y API, servicios/handlers, QA extendida para modo v2                  |
| Sprint 3-v2   | 2                | PENDING     | Integraciones periféricas, protocolos extendidos (BLE/OBD2), QA, HTTP Monitor v2  |
| Sprint 1-v25  | 2.5              | PENDING     | Upgrade parser v2.5, features exclusivos (seguridad, variantes, multiplexing)      |
| Sprint 2-v25  | 2.5              | PENDING     | Integraciones avanzadas, testeo interoperabilidad ConfIoT y Layrz Suite            |
| Sprint 3-v25  | 2.5              | PENDING     | Optimización, microajustes QA final, candidate                                    |


## Detalle de cada Sprint (por versión)

### Sprints Layrz v1
- S1-v1: pool buffers, CRC16 (ROM), parser FSM v1, QA mínima v1
- S2-v1: Message Bus/config persistente, Ownership contract, glue v1
- S3-v1: Servicios C++17, decoders, auto-discovery only v1, validación strict v1
- S4-v1: Testing edge/recovery, soak tests, Safe Mode solo v1

### Sprints Layrz v2
- S1-v2: Adaptación parser a v2, QA migración, features sólo para versión 2
- S2-v2: Servicios nuevos (handlers, messagebus), integraciones, QA v2
- S3-v2: Protocolos extendidos, periféricos, HTTP Monitor salida v2

### Sprints Layrz v2.5
- S1-v25: Upgrade parser a 2.5, soporte multiplexing, bloques especiales
- S2-v25: Integraciones ConfIoT/Layrz Suite, interoperabilidad y testing
- S3-v25: Optimización, métricas, QA y validación completas para release

---

**Notas:**
- Todo feature/QA/doc debe especificar la versión Layrz exacta a la que aplica.
- Los features rotulan "v1"/"v2"/"v2.5". Las versiones no se mezclan por sprint salvo gates explícitos.
- El estado se debe actualizar por bloque (por version) sólo al concluir auditoría y verificación QA de esa versión.
- Mantener la trazabilidad ISO y evidencia de feature/QA en /docs/sprints/feature_*/ y /pilot_results/

(Actualizado para gating y releases secuenciales de Layrz v1 → v2 → v2.5)
