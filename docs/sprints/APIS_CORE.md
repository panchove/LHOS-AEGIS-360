# APIs C/C++ Core – LHOS-AEGIS-360

Este documento define las interfaces principales del core (C17/C++17) y el contrato de persistencia en GZIP propio obligatorio para toda blackbox/logging.

---

## 1. Buffer Pool (pool.h)

- Inicialización y stats:
  - `int pool_init(void);`       // Inicializa el pool, devuelve 0 en éxito
  - `size_t pool_free_count(void);` // Devuelve número de buffers libres
- Asignación/liberación:
  - `pool_buf_t* pool_alloc(void);` // Asigna buffer, devuelve ptr o NULL
  - `void pool_release(pool_buf_t* buf);` // Libera buffer (ownership), seguro para doble release
- Contrato: el ownership SIEMPRE debe transferirse explícitamente. El receptor es responsable de pool_release.

---

## 2. CRC (crc16.h)

- `uint16_t crc16_ccitt(const uint8_t *data, size_t len);`
- Pura en C, sin dependencias externas.
- Contrato: siempre usar para cierre de paquete, previniendo corrupción de datos.

---

## 3. Parser (parser.h)

- Inicialización:
    - `typedef void (*parser_dispatch_cb_t)(pool_buf_t* buf);`
    - `int parser_init(parser_dispatch_cb_t cb);`
- Alimentación:
    - `void parser_feed_byte(uint8_t b);`
    - `void parser_feed_buf(const uint8_t* buf, size_t len);`
- Contrato: la FSM recibe bytes y entrega pool_buf_t* válidos sólo si CRC y delimitadores son correctos. Si ocurre error, el buffer se libera automáticamente.

---

## 4. Message Bus (message_bus.h)

- Publicación:
    - `int message_bus_publish(pool_buf_t* buf);` // Ownership transfer
- Suscripción (opcional/para test):
    - `void message_bus_subscribe(cb_fn);`
- Contrato: el bus adquiere ownership absoluto tras publish, el publisher no debe hacer release.

---

## 5. Blackbox/Logging (blackbox.h, GZIP own impl)

- Enqueue (persistente, GZIP):
  - `int blackbox_enqueue(const uint8_t* data, size_t len);` // Persiste automáticamente en GZIP propio.
- Lectura/secuenciamiento:
  - `int blackbox_read_next(uint8_t* out_buf, size_t* out_len);` // Lee y descomprime el siguiente registro.
- Stats:
  - `int blackbox_get_stats(bb_stats_t* stats);` // Entrega tamaño jsn/gz etc.
- Contrato: TODA escritura/lectura es en GZIP propio interno del repo. Está prohibido almacenar blackbox/raw sin compresión propietaria.
  - Para cualquier feature HTTP/descarga, el archivo es servido en formato gzip con header `Content-Encoding: gzip` si corresponde.

---

> NOTA: Cada header o módulo deberá documentar explícitamente la no dependencia legacy y el uso obligatorio de contratos formales/ownership.
