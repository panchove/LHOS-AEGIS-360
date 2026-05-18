// message_bus.h – Bus contractual (Sprint 01, ODIN, heapless)
// Contract: ownership/raw, pool integr, QA hooks
#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H
#include <stddef.h>
#include "pool.h"

typedef void (*message_bus_cb_t)(pool_buf_t*);
// Inicializa bus, pool opcional
int message_bus_init(void);
// Publish buffer (ownership transfer), retorna 0 en éxito
int message_bus_publish(pool_buf_t* buf);
// Subscribe (solo 1 callback demo, sprint01)
void message_bus_subscribe(message_bus_cb_t cb);
// QA: cuentas
size_t message_bus_publish_count(void);
size_t message_bus_dispatch_count(void);

#endif // MESSAGE_BUS_H
