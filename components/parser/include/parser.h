#pragma once
#include <stddef.h>
#include <stdint.h>
#include "pool.h"

typedef void (*parser_dispatch_cb_t)(pool_buf_t* buf);

int parser_init(parser_dispatch_cb_t cb);
void parser_feed_byte(uint8_t b);
void parser_feed_buf(const uint8_t* buf, size_t len);
