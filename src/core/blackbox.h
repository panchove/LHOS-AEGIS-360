// blackbox.h – API blackbox/logging contractual (Sprint 02, ODIN)
#ifndef BLACKBOX_H
#define BLACKBOX_H
#include <stdint.h>
#include <stddef.h>
#include "pool.h"

// Encola datos en blackbox con compresión, ownership transfer
int blackbox_enqueue(const uint8_t* data, size_t len);
// Lee/descomprime siguiente registro
int blackbox_read_next(uint8_t* out_buf, size_t* out_len);
// Stats QA
size_t blackbox_get_stats(void);

#endif // BLACKBOX_H
