# Implementation Plan — Core en C17, capas superiores en C++17

Objetivo: definir interfaces y contratos para la re-implementación limpia del firmware tomando requisitos de legacy_reference. El CORE (componentes críticos: pool, parser, message bus minimal, transport glue) se implementará en C17. Las capas superiores (servicios, decoders, tests y utilidades) podrán usar C++17 sin exceptions/RTTI.

1) Contratos generales
- Ownership: pool alloc → devuelve pool_buf_t*; la llamada obtiene la propiedad. La transferencia de ownership se efectúa explícitamente al MessageBus mediante MessageBus_publish(buf) (C API). Quien reciba ownership debe llamar pool_release(buf) cuando termine.
- Heap policy: buffers grandes almacenados en PSRAM. El código core puede usar heap_caps_malloc con MALLOC_CAP_SPIRAM para reservas controladas durante init; en runtime evitar malloc/free repetidos.
- Sin exceptions/RTTI, no std::vector/strings en core. C structs y arrays estáticos o PSRAM-backed.

2) Componentes core (C17)
- components/pool: pool_init(), pool_alloc(), pool_release(), pool_stats(). Implementación lock-free con atomic_flag por slot (fast). Buffers en PSRAM.
- components/crc: crc16_ccitt() pura en C, con tabla (fast) y test vectors.
- components/parser: parser_init(dispatch_cb), parser_feed(byte), parser_feed_buf(ptr,len). Parser FSM assemble bytes into pool buffer, detect delimiter ';' y validar CRC16 si el final incluye 4 hex digits. Si ok, llama dispatch_cb(pool_buf_t*). En fallo, libera buffer.
- components/message_bus (esqueleto): C API que acepta ownership. (Se implementará después.)

3) Capas superiores (C++17)
- services/layrz_protocol: command dispatcher (thin), handlers, interactúa con MessageBus C API.
- devices/decoders: decoders BLE/serial en C++17, retornan JSON o struct simple.

4) Testing
- tests/unit: Unity-based tests para crc16 y pool (alloc/release), y pruebas del parser FSM. Cada test es un component compatible con idf/unity.

5) CI / contract checks
- tools/check_contracts.sh: grep estático para prohibidos (malloc/new/delete, std::vector) en archivos core/.

6) Next actions (implementación inmediata)
- Añadir componentes: pool (C), crc (C), parser (C) con tests unitarios mínimos.
- Integrar CMakeLists por componente para que `cmake` los detecte.

APIs propuestas (simplificadas)

// pool/include/pool.h (C)
typedef struct pool_buf_t pool_buf_t;
int pool_init(void);
pool_buf_t* pool_alloc(void);
void pool_release(pool_buf_t* buf);
size_t pool_free_count(void);

// crc/include/crc16.h (C)
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

// parser/include/parser.h (C)
typedef void (*parser_dispatch_cb_t)(pool_buf_t* buf);
int parser_init(parser_dispatch_cb_t cb);
void parser_feed_byte(uint8_t b);
void parser_feed_buf(const uint8_t* buf, size_t len);

Si estás de acuerdo, en el siguiente paso crearé los archivos fuente y los tests iniciales para pool/crc/parser.
