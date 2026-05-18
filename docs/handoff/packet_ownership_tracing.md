# Packet Ownership Tracing (Trazabilidad de Ownership de Paquetes)

Propósito
- Documentar de forma contractual quién posee, transfiere y libera buffers/paquetes a lo largo del pipeline (ISR → Transport → Parser → Dispatcher → Consumer). Este documento cumple la necesidad de trazabilidad del modelo packet-centric y mapea responsabilidades a APIs concretas.

Alcance
- Aplica a todos los pipelines de transporte y protocolo que entregan paquetes a la capa de aplicación. Incluye UART, BLE, TCP y adaptadores similares.

Definiciones
- Paquete: unidad lógica contractual completa que viaja desde Parser hasta Consumer.
- Ownership: responsabilidad explícita de liberar o devolver el buffer al pool.
- Pool: conjunto de buffers estáticos (heapless) provistos por el sistema.

Principios contractuales
1. Single Owner: cada buffer tiene en todo momento un único owner. El owner solo cambia mediante handoff explícito.
2. Move-Only Semantics: ownership se mueve (no se copia) entre etapas; cualquier copia física debe crear su propio buffer/alloc y documentarse.
3. No free en ISR: ISR nunca libera buffers; solo encola referencia a tarea que tomará la responsabilidad.
4. Release Authority: cada stage tiene autoridad explícita para liberar (ver tabla).

Tabla de Ownership (Stage → Owner → Release Authority)
 - ISR                → ISR Handler (referencia) → transport task (on dequeue)
 - Transport (frame)  → transport_core          → transport_core (on send to parser)
 - Parser/Protocol    → protocol_core          → protocol_core (on success→dispatcher_submit or on failure→release/quarantine)
 - Dispatcher         → dispatcher             → consumer/service (on ack) or dispatcher (after delivery timeout)
 - Consumer/Service   → consuming_service      → consuming_service (release to pool when done)

Handoff API Contracts (ejemplos C)
- packet_buf_t* pool_alloc(void);
- esp_err_t pool_release(packet_buf_t* p);
- esp_err_t dispatcher_submit(packet_buf_t* p); // transfer ownership to dispatcher

Reglas de Handoff (obligatorias)
1. Cuando parser llama dispatcher_submit(p): ownership queda transferida. Parser no debe acceder a p después de la llamada.
2. Dispatcher entrega el buffer al consumidor; el consumidor es responsable de liberarlo usando pool_release() cuando termina.
3. En caso de error detectado en cualquier stage (MALFORMED/TIMEOUT/CRC_FAIL) el stage que detecta el fallo debe liberar el buffer y registrar el evento.
4. Todas las llamadas a pool_release deben ser idempotentes en la medida de lo posible (chequear validación y setear pointer a NULL en código consumidor).

Instrumentación y trazabilidad
- Cada operación de alloc/release debe emitir un evento ligero (circular buffer en memoria no heap) con: timestamp, file:line, operation (alloc/release), owner_id, buffer_id.
- Métricas a exponer:
  - pool_free_count
  - alloc_rate (ops/min)
  - release_rate (ops/min)
  - double_release_count
  - parser_timeouts

Diagnóstico y Runbook rápido
- Si pool_free_count cae sostenidamente: activar RUNBOOK_PACKET_LEAK_DIAGNOSIS.md (dump trace, habilitar heap debug, capturar stacks de release).

Appendix: Ejemplo de flujo
1. UART ISR recibe bytes, encola referencia o bytes en ring buffer.
2. transport_task (owner) extrae bytes, agrupa en frames, llama parser_feed() y pasa bytes al parser task.
3. parser_task hace pool_alloc() → llena buffer → al completar y validar CRC llama dispatcher_submit(buffer).
4. dispatcher_task recibe pointer y encola a consumer task. Dispatcher ya no posee buffer.
5. consumer_task procesa y llama pool_release(buffer) cuando termina.

Control de cumplimiento
- Revisión de código y CI contract check (tools/contract_check.sh) deben buscar patrones prohibidos: free/malloc/new en módulos core, uso de std::vector en runtime, y validar que dispatcher_submit es el único camino de transfer ownership.
