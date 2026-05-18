# Modelo centrado en Paquetes (Packet-centric)

- El sistema heapless funciona sólo si cada paquete tiene estado y trazabilidad exhaustivos.
- Cualquier falta de transición válida, doble liberación o acceso fuera de secuencia se detecta con bug crítico.
- Dispatcher: nunca toma posesión del paquete, sólo encola/ruta (READY → DISPATCHED). Nunca muta pool/lifetime.
- Parser: único autorizado a marcar MALFORMED/TIMEOUT. Cierre requiere RELEASED. HANDOFF contractual en docs/handoff/packet_ownership_tracing.md.
- Testing: packet_lifetime_tracing_test.cpp cubre todos los casos borde del modelo.
