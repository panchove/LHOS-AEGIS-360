#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../crc/include/crc16.h"

// Simple parser: collects until ';' (delimiter). On terminator, validates optional
// CRC marker of last 4 hex chars (if present) and dispatches buffer.

static parser_dispatch_cb_t g_dispatch_cb = NULL;
static pool_buf_t *g_current = NULL;

int parser_init(parser_dispatch_cb_t cb) {
  if (!cb) return -1;
  g_dispatch_cb = cb;
  return 0;
}

void parser_feed_byte(uint8_t b) {
  if (!g_current) {
    g_current = pool_alloc();
    if (!g_current) return; // drop on OOM
  }
  // append if space
  if (g_current->len + 1 < 512) {
    ((uint8_t*)g_current->data)[g_current->len++] = b;
  } else {
    // overflow -> drop
    pool_release(g_current);
    g_current = NULL;
    return;
  }
  if (b == ';') {
    // packet end
    // check for CRC suffix pattern: last 4 hex chars before ';' (simple)
    size_t payload_len = g_current->len;
    // remove trailing ';' for crc calculation if needed
    if (payload_len >= 5) {
      // attempt to compute CRC over content excluding last 4 chars
      // naive: if last 5 chars looks like "XXXX;" where X are hex, treat last 4 as CRC
      bool hex4 = true;
      for (size_t i = payload_len - 5; i < payload_len -1; ++i) {
        char c = ((char*)g_current->data)[i];
        if (!((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f'))) { hex4 = false; break; }
      }
      if (hex4) {
        // compute CRC
        size_t data_len = payload_len - 5; // exclude 4 hex + ';'
        uint16_t calc = crc16_ccitt((const uint8_t*)g_current->data, data_len);
        // parse provided CRC
        char provided[5] = {0};
        for (int i=0;i<4;i++) provided[i] = ((char*)g_current->data)[data_len + i];
        unsigned int val=0;
        if (sscanf(provided, "%4x", &val)==1) {
          if ((uint16_t)val != calc) {
            // CRC fail -> drop
            pool_release(g_current);
            g_current = NULL;
            return;
          }
        }
      }
    }
    // deliver
    if (g_dispatch_cb) {
      g_dispatch_cb(g_current);
    } else {
      pool_release(g_current);
    }
    g_current = NULL;
  }
}

void parser_feed_buf(const uint8_t* buf, size_t len) {
  for (size_t i =0;i<len;i++) parser_feed_byte(buf[i]);
}
