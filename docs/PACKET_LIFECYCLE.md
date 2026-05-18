# PAQUETE: CICLO DE VIDA Y TACEABILIDAD

## Estados posibles (heapless contract)
- ALLOCATED   (pool: adquirido)
- IN_PARSER   (parsing)
- READY       (pre-dispatch)
- DISPATCHED  (routed/entregado handler)
- CONSUMED    (procesado/capturado handler)
- RELEASED    (regresado a pool)
- DROPPED     (descartado, no usado)
- MALFORMED   (parser detecta inválido)
- TIMEOUT     (parser detecta timeout)

## Reglas
- [CORREGIDO] Regla unificada de ownership: MessageBus toma ownership al publicarse el buffer. Dispatcher y Handlers no liberan.
- Todas las transiciones forzadas a perfil contractual
- Doble release, acceso tras RELEASED/DROPPED causa bug crítico
- Dispatcher NO cambia ownership, sólo READY→DISPATCHED
- Parser puede marcar MALFORMED/TIMEOUT (requiere RELEASED posterior por MessageBus)
- Sin registro de estado (tracing heapless): bug crítico, bloquea release

## Ejemplo pipeline
ALLOCATED → IN_PARSER → READY → DISPATCHED → CONSUMED → RELEASED
ALLOCATED → IN_PARSER → MALFORMED → RELEASED
ALLOCATED → IN_PARSER → TIMEOUT → RELEASED
ALLOCATED → DROPPED

## Test y métricas
Véase packet_lifetime_tracing_test.cpp; cobertura exhaustiva en CI con `idf.py build && idf.py -T all test`.
