# ---
owner: ODIN
last_reviewed: 2026-05-18
status: draft
doc_id: PACKET_CENTRIC_MODEL
# ---

# Modelo centrado en Paquetes (Packet-centric)

[CORREGIDO] Regla de ownership unificada: el MessageBus toma ownership cuando se publica el buffer. Dispatcher y Handlers solo leen.

- El sistema heapless funciona sólo si cada paquete tiene estado y trazabilidad exhaustivos.
- Cualquier falta de transición válida, doble liberación o acceso fuera de secuencia se detecta con bug crítico.
- Ownership:
	- Parser: crea el buffer (`pool_alloc`) y, tras validar (CRC ok), llama `dispatch_cb(buf)`.
	- MessageBus: cuando se llama `MessageBus_publish(buf)` toma ownership y será responsable de `pool_release(buf)`.
	- Dispatcher y Handlers: no toman ownership; solo leen referencias al buffer.
- Parser: único autorizado a marcar MALFORMED/TIMEOUT. Cierre requiere RELEASED. HANDOFF contractual en docs/handoff/packet_ownership_tracing.md.
- Testing: packet_lifetime_tracing_test.cpp cubre todos los casos borde del modelo.
