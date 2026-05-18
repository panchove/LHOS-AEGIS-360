# Implementation Plan — Core en C++17 (decisión: CORE en C++17)

[CORREGIDO] Decisión de arquitectura: el Core/runtime se implementará en C++17 (sin exceptions, sin RTTI). C queda reservado exclusivamente para ISR/HAL/boot glue y drivers estrictamente necesarios.

Objetivo: definir interfaces y contratos para la re-implementación limpia del firmware tomando requisitos de legacy_reference. El CORE (componentes críticos: pool, parser, message bus minimal, transport glue) se implementará en C++17. Las capas superiores (servicios, decoders, tests y utilidades) podrán usar C++17 sin exceptions/RTTI.

1) Contratos generales
- Ownership (DECISIÓN UNIFICADA):
	- Parser: crea el buffer (`pool_alloc`) y tras validar (CRC ok) llama `dispatch_cb(buf)`.
	- MessageBus: *toma ownership* cuando se llama `MessageBus_publish(buf)`. MessageBus es responsable de encolar, retransmitir y finalmente llamar `pool_release(buf)` cuando ya no sea necesario.
	- Dispatcher y Handlers: *no toman ownership*; solo leen el buffer. El único componente autorizado para liberar es el MessageBus/BlackBox.

- Heap policy: buffers grandes almacenados en PSRAM. El código core en C++17 debe usar pools y allocators controlados; sólo permitir allocaciones controladas durante init.
- Sin exceptions/RTTI en el core; usar RAII y wrappers ligeros para ownership. Evitar `std::vector`/`std::string` en hot-paths; preferir span-like views y buffers PSRAM-backed.

2) Componentes core (C++17)
- components/pool: `pool_init()`, `pool_alloc()`, `pool_release()`, `pool_stats()`. Implementación en C++17 con wrappers RAII y estructuras POD para las slots; buffers en PSRAM.
- components/crc: `crc16_ccitt()` como función `constexpr`/referencia en C++ con tabla precomputada y vectores de test.
- components/parser: `parser_init(dispatch_cb)`, `parser_feed_byte`, `parser_feed_buf`. FSM en C++17 que ensambla bytes en buffers del pool; al completar y validar CRC llama `dispatch_cb(pool_buf_t*)` transfiriendo ownership al MessageBus.
- components/message_bus: interfaz C++ (con bindings C si es necesario) que acepta ownership y es responsable de liberar buffers (`pool_release`) tras envío o persistencia en BlackBox.

3) Capas superiores (C++17)
- services/layrz_protocol: command dispatcher (thin), handlers, interactúa con MessageBus C API.
- devices/decoders: decoders BLE/serial en C++17, retornan JSON o struct simple.

4) Testing
- tests/unit: Unity-based tests para crc16 y pool (alloc/release), y pruebas del parser FSM. Cada test es un component compatible con idf/unity.

5) CI / contract checks
- tools/check_contracts.sh: grep estático para prohibidos (malloc/new/delete, std::vector) en archivos core/.

6) Next actions (implementación inmediata)
- Añadir componentes: pool (C++17), crc (C++17), parser (C++17) con tests unitarios mínimos.
- Implementar bindings C sólo donde sea necesario (ISR/drivers/boot glue).
- Integrar CMakeLists por componente para que `idf.py`/CMake los detecte.

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
