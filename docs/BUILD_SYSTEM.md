# BUILD SYSTEM

## Política de Stack y Librerías (Espressif/ESP-IDF)
- Prohibido el uso de cualquier librería Arduino y de terceros fuera de los componentes oficiales Espressif.
- Solo se permite ESP-IDF v5.5.3 como framework, base de compilación y runtime.
- Tiempo de sistema debe sincronizarse obligatoriamente con HASTA 2 servidores NTP.
- PRIORIDAD: El uso de servidores NTP enviados por ConfIoT solo será efectivo a partir del Sprint de integración con ConfIoT. Hasta ese punto, únicamente se autorizan y utilizan los internos: 'pool.ntp.org' y 'time.google.com'. La UI debe mostrar la fuente activa ('NTP/Default'). Cuando se active ConfIoT sólo se usan máximo 2 entregados.
- Si llegan más de 2 desde ConfIoT solo se usan los 2 primeros.
- Fallback automático entre los NTP; si ambos fallan, dejar log crítico y alertar en UI.
- Formato global de fecha/hora: UTC ISO-8601. Monitor/UI muestra 'AAAA-MM-DD HH:MM UTC (PAIS-ISO)'.
- Si no es posible determinar el país por red local/RTC/GPS, debe llamarse a una API pública (como myip.com o ipapi.co) para detectar y mostrar el código país ISO.

- Cualquier excepción o uso de dependencia externa requiere autorización y documentación previa por Arquitectura.

## C++17 Core Policy
- Core/runtime: C++17 estricto, heapless, sin exceptions ni RTTI.
- Permitido: enum class, constexpr, templates, interfaces POD, std::array (sin heap), RAII determinista.
- Prohibido: exceptions, RTTI, std::function, std::vector/map/set en core, new/delete arbitrario, ownership ambiguo.
- C sólo en glue ESP-IDF y drivers/HAL.

## Test Integration (ESP-IDF/Unity)
- All tests deben definirse como components ESP-IDF usando `idf_component_register`.
- Manual `add_executable` targets are prohibited.
- Cada test suite en `tests/<suite>` con `CMakeLists.txt` usando `idf_component_register`.
- Include Unity en `REQUIRES`.
- Build y test using:
  - `idf.py fullclean`
  - `make test`

---

## Filesystem/Storage Policy
- Únicamente LittleFS está permitido como filesystem persistente en QA, producción y pruebas.
- Está prohibido el uso de SPIFFS, FATfs u otros filesystems, salvo para almacenamiento temporal volátil explícitamente documentado.
- Justificación: SPIFFS tiene alto riesgo de corrupción y ha sido reemplazado en la plataforma ESP-IDF/Layrz.

## Core/Runtime Heapless Policy (Sprint 03)
- Core runtime components (BootOrchestrator, ServiceRegistry, DependencyGraph, EventBus, TaskRegistry, HealthRegistry, WatchdogRegistry, Logger core) son estrictamente heapless.
- Forbidden constructs deben detectarse mediante check-contract script.
