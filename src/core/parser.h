// parser.h – FSM de parser de paquetes (Sprint 01)
// Contrato: ownership, heapless, integración QA/metrics
// Referencia: APIS_CORE.md sección 3, FEATURE_parser_fsm.md
#ifndef PARSER_H
#define PARSER_H
#include <stddef.h>
#include <stdint.h>
#include "pool.h"

typedef void (*parser_dispatch_cb_t)(pool_buf_t* buf);

// Inicializar parser y asociar callback (ownership de buf propio del parser)
int parser_init(parser_dispatch_cb_t cb);
// Alimentación por byte
void parser_feed_byte(uint8_t b);
// Alimentación por buffer
void parser_feed_buf(const uint8_t* data, size_t len);
// QA / métricas
size_t parser_error_count(void);
size_t parser_dispatch_count(void);

#endif // PARSER_H
