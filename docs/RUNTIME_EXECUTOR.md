# Runtime Executor

Define el contexto heapless, FreeRTOS-aware y determinista para la ejecución de servicios y tasks bajo AEGIS.

- Orquesta contextos de ejecución, tick, heartbeat propagation, y coordinación de tareas.
- Garantiza políticas de afinidad (por servicio), modo (ACTIVE_LOOP, PERIODIC, EVENT_DRIVEN, PASSIVE), prioridad y policy de heartbeat.
- Sin ownership ambiguo, sin service locator, sin alloc/free/xTaskCreate.

## Responsabilidades
- Crear contextos controlados
- Registrar tasks (stacks estáticos, StaticTask_t)
- Coordinar runtime tick y heartbeat
- Propagar eventos métricos y de backpressure

## Prohibiciones
- NO crea servicios de forma arbitraria
- NO own/resuelve dependencias ni service location
- Nunca heap ni malloc, nunca xTaskCreate
