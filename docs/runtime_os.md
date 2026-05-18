# Runtime OS Specification (FreeRTOS) — Core C++17

[CORREGIDO] El Core/runtime se implementa en C++17 (sin exceptions/RTTI). C se reserva para ISR/HAL/boot glue.

Resumen
- RTOS: FreeRTOS (usado a través de ESP-IDF) sobre ESP32-S3.
- Implementación: componentes CORE en C++17; capas superiores en C++17 sin exceptions/RTTI.
- Filosofía: determinismo, ownership explícito, evitar malloc/new en hot-paths.

Objetivos operativos
- Tasks críticas pinneadas a core 1.
- No creación dinámica de tareas en el CORE durante operación normal; creación preferente en init usando stacks estáticos.
- Comunicación: Zero-copy con pool_buf_t* en queues.
- Observabilidad: heartbeats por tarea, contador de fallos, watchdogs locales.

Task Registry (concepto)
- Un componente mínimo que permite:
  - Registrar tareas críticas con un periodo/timeout de heartbeat.
  - Registrar pings (heartbeats) desde las tareas.
  - Monitoreo periódico que detecta tareas que no hacen ping dentro del timeout y notifica.
  - API auxiliar para crear tareas pinneadas (con advertencia: preferir creación estática en init para el CORE).

Política de uso
- Cada tarea crítica debe llamar task_registry_ping(handle) periódicamente (por ejemplo, en su bucle principal o antes de bloquear por > timeout/2).
- El monitor decide acciones: log, bandera para Safe Mode, o llamar al Watchdog supervisor.

APIs principales (resumen)
- task_registry_init(monitor_period_ms): inicializa el monitor y su tarea.
- create_pinned_task(fn, name, stack_size, prio, core, param): crea y devuelve TaskHandle_t.
- task_registry_register_heartbeat(handle, name, timeout_ms): registra una tarea para monitor.
- task_registry_ping(handle): actualiza el timestamp de heartbeat.

Health policy
- Parámetros por defecto recomendados:
  - monitor_period_ms = 1000
  - task heartbeat timeout: 5000 ms para tareas críticas, 10000 ms para tareas medias.
- En caso de expiración: reportar al logging, incrementar contador crash_watchdog_kicks y opcionalmente activar safe-mode.

Integración con Watchdog
- Las tareas críticas deben además usar esp_task_wdt_add/esp_task_wdt_reset si implementan cargas que requieran watchdog de tarea.
- El Task Registry es complementario: detecta stalls lógicos donde el watchdog HW pueda seguir sin detectar (ej. deadlocks en recursos compartidos).

Notas de implementación
- El primer deliverable incluye un Task Registry minimal en C con monitor activo, API de registro y helpers para crear tareas pinneadas.
- Futuras mejoras: tareas estáticas creadas con StaticTask_t y buffers de stack estáticos, política de escalado en caso de múltiples expiraciones, telemetría integrada.
