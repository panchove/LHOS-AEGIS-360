# Boot Flow & Estado — AEGIS Runtime

## Boot Paths

1. Normal Boot
    - Estado: `CREATED` → `INITIALIZING` → `READY`
    - Todos los servicios y registries pasan check de dependencia, pasan health check, sin quarantine.
2. Safe Mode Boot
    - Estado: `CREATED` → `INITIALIZING` → `DEGRADED`
    - Servicios críticos solamente.
3. Degraded Boot
    - Estado: `CREATED` → `INITIALIZING` → `DEGRADED`
    - Uno o más servicios fallan o quedan en quarantine; el resto opera en modo reducido.
4. Crash Recovery Boot
    - Estado: `CREATED` → `INITIALIZING` → `DEGRADED`
    - Se detectan fallas previas, se arranca lo mínimo en modo degradado.

## Transición de Estados (por BootOrchestrator)

```
    CREATED
      |
      v
    INITIALIZING
      |
      v
    READY
      |    ↘
      v     \
   STOPPED   DEGRADED
      ^        |
      |        v
   FAILED  ←QUARANTINED
```
