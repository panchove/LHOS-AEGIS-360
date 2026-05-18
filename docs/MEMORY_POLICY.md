# LHOS-AEGIS-360 — Memory Policy (Reliability/Recovery)

## C++17 Core Policy (Sprint 03)

- Todo el core/runtime AEGIS debe ser C++17 puro, heapless, determinista.
- PERMITIDO: enum class, constexpr, templates, structs POD, interfaces abstractas simples
- PROHIBIDO: exceptions, RTTI, std::function, std::vector/map/set en core
- C permitido sólo para glue ESP-IDF, ISR/HAL/drivers C, o APIs C.

## Memory pressure/failure
- PSRAM exhaustion fuerza MEMORY_PRESSURE_HIGH
- Queue overflow igual
- Entra recovery/degrade, nunca loop
- Crash/boot reason en NVS

## Heap Policy / Core Heapless Enforcement

SPRINT 03: Runtime/core = heapless absoluto.

En todo módulo core/critical está completamente PROHIBIDO:
- malloc/free
- new/delete
- std::vector
- std::map
- std::set
- std::function
- heap por evento
- heap en ISR

PERMITIDO (fuera del core, futuro):
- Allocación controlada en boot/init
- PSRAM-backed pools, buffer pools, TLS/BLE/HTTP/media buffers etc, sólo justificados/documentados

Reglas de oro:
- No alloc/free repetitivo en runtime crítico
- Asignar una vez, reutilizar siempre
