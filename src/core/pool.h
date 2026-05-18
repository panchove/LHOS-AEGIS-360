// pool.h – Heapless buffer pool (Sprint 01, core)
// Contrato ISO/QA: ownership explícito, heapless policy, métricas integradas
// Referencia: APIS_CORE.md sección 1, FEATURE_pool_heapless.md
//
// Toda persistencia/log debe ser sobre LittleFS, QA/check-contract obligatorio
// Política: no std::vector/string, sin heap/new/delete, C17 puro
#ifndef POOL_H
#define POOL_H
#include <stddef.h>
#include <stdint.h>

#define POOL_BUF_SIZE  256
#define POOL_BUF_COUNT 16

typedef struct {
    uint8_t data[POOL_BUF_SIZE];
    size_t  len;
    uint32_t magic;
} pool_buf_t;

// Inicializa el pool, retorna 0 en éxito
int pool_init(void);
// Devuelve número de buffers libres
size_t pool_free_count(void);
// Asigna buffer o NULL si agotado
pool_buf_t* pool_alloc(void);
// Libera buffer, seguro para doble release
void pool_release(pool_buf_t* buf);
// Estadísticas (usado en QA/tests)
size_t pool_high_water_mark(void);
size_t pool_total_count(void);

#endif // POOL_H
