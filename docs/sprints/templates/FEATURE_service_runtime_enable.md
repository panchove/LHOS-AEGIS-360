# Feature – Gestión dinámica y persistente de servicios (enable/disable por ConfIoT/Monitor)

## 1. Descripción breve
Permite habilitar o deshabilitar servicios individuales (tasks, handlers, drivers, APIs) en tiempo real por ConfIoT o Monitor, con persistencia en almacenamiento no volátil y restauración automática en el siguiente reinicio.

## 2. Contrato y referencias
- ISO12207_GUIDELINES.md (sección servicios dinámicos)
- APIs: /api/v1/svc (GET/POST)
- QA: logs, tests de persistencia y recuperación

## 3. Requisitos funcionales
- [ ] Permitir habilitar/deshabilitar cada servicio vía Monitor (HTTP) y ConfIoT remota
- [ ] Proteger (no exponer toggle, no modificar ni por error) los servicios indeactivables/core: WDT, heap checker, tick, recovery, API mínima, config starter, y compresión GZIP (marcar GZIP como indeactivable: crítico para ahorro de storage, integridad logs).
- [ ] Establecer estructura de estado corta y compacta: {"svc":1, "mod":0, ...}, donde los indeactivables faltan o son readonly (always-on)
- [ ] Guardar cambios en NVS/archivo gzip y restaurar al reiniciar
- [ ] Reflejar en UI y API el estado actual, fuente del cambio y protección de mandatorios (tag “always-on”, no toggleable).

## 4. No funcionales
- [ ] Cambios en caliente, sin reboot salvo fuerza mayor
- [ ] Trazabilidad (log) de fuente/cambio/timestamp

## 5. Arquitectura/diseño
- Bitmask/mapa cross-service, guardado comprimido en flash/NVS
- Hooks en monitor/ConfIoT, endpoint /api/v1/svc
- Nivel de fuente (manual vs remoto) y resolución de conflictos

## 6. Checklist de entregables
- [ ] Nuevo endpoint/handler monitor/ConfIoT (GET/POST)
- [ ] Persistencia implementada y testeada (exclusivamente en LittleFS; SPIFFS queda prohibido y debe fallar QA en caso de detectarse)
- [ ] Ejemplos en QA y monitor UI actualizada
- [ ] Documentación contract/CHANGELOG

## 7. Criterios QA
- [ ] Cambios vía API/Monitor/ConfIoT se aplican y persisten correctamente
- [ ] Arranque sólo de servicios activos; correctos fallbacks ante fallo de persistencia
- [ ] Cambios logueados con fuente
- [ ] Intento de deshabilitar servicios indeactivables es ignorado, logueado y el servicio queda activo (QA obligatorio)
- [ ] UI y API marcan "always-on" servicios protegidos, sin toggle


## 8. Backwards/compatibilidad
- [ ] Compatible con estados previos
- [ ] Default seguro en caso de corrupción/upgrade

## 9. Observaciones
Incluye matriz de pruebas concurrentes, edge cases y plan de rollback tras bugs críticos.
