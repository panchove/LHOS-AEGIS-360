#include "pool.h"
#include <string.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <esp_heap_caps.h>

// Config
#define POOL_SLOT_COUNT 16
#define POOL_SLOT_SIZE  512

typedef struct slot_s {
  atomic_uint locked; // 0=free, 1=locked
  unsigned id;
  void *buf; // PSRAM allocated
} slot_t;

static slot_t g_slots[POOL_SLOT_COUNT];

#include <esp_log.h> // Añadir logging

int pool_init(void) {
  ESP_LOGI("pool", "Inicializando pool con %u slots de %u bytes (%.1f KB total)", POOL_SLOT_COUNT, POOL_SLOT_SIZE, (POOL_SLOT_COUNT*POOL_SLOT_SIZE)/1024.0);
  for (unsigned i = 0; i < POOL_SLOT_COUNT; ++i) {
    atomic_init(&g_slots[i].locked, 0u);
    g_slots[i].id = i;
    size_t free_caps = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_LOGI("pool", "Slot %u: free SPIRAM=%u", i, (unsigned)free_caps);
    g_slots[i].buf = heap_caps_malloc(POOL_SLOT_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g_slots[i].buf) {
      ESP_LOGE("pool", "Fallo malloc slot %u (necesita %u bytes), SPIRAM disponible=%u. Corrige PSRAM/config/particion.", i, POOL_SLOT_SIZE, (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      // free previously allocated
      for (unsigned j = 0; j < i; ++j) {
        heap_caps_free(g_slots[j].buf);
        g_slots[j].buf = NULL;
      }
      return -1;
    }
    memset(g_slots[i].buf, 0, POOL_SLOT_SIZE);
  }
  ESP_LOGI("pool", "pool_init OK");
  return 0;
}


pool_buf_t* pool_alloc(void) {
  for (unsigned i = 0; i < POOL_SLOT_COUNT; ++i) {
    unsigned expected = 0u;
    if (atomic_compare_exchange_strong(&g_slots[i].locked, &expected, 1u)) {
      // acquired
      pool_buf_t *p = (pool_buf_t*)heap_caps_malloc(sizeof(pool_buf_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!p) {
        atomic_store(&g_slots[i].locked, 0u);
        return NULL;
      }
      p->data = g_slots[i].buf;
      p->len = 0;
      p->id = g_slots[i].id;
      return p;
    }
  }
  return NULL;
}

void pool_release(pool_buf_t* buf) {
  if (!buf) return;
  unsigned id = buf->id;
  if (id >= POOL_SLOT_COUNT) {
    heap_caps_free(buf);
    return;
  }
  // zero buffer for safety
  memset(g_slots[id].buf, 0, POOL_SLOT_SIZE);
  atomic_store(&g_slots[id].locked, 0u);
  heap_caps_free(buf);
}

unsigned pool_free_count(void) {
  unsigned c = 0;
  for (unsigned i = 0; i < POOL_SLOT_COUNT; ++i) {
    if (atomic_load(&g_slots[i].locked) == 0u) ++c;
  }
  return c;
}
