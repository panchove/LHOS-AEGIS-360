#pragma once
#include "esp_err.h"
#include "pool.h"

esp_err_t dispatcher_init(void);
// Parser calls this to handoff buffer ownership
esp_err_t dispatcher_submit(packet_buf_t* packet);
