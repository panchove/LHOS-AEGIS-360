#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "pool.h"
#include "transport.h"
#include "parser.h"
#include "dispatcher.h"

static const char *TAG = "BootOrchestrator";

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "BootOrchestrator start");

    esp_err_t err;

    err = pool_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pool_init failed: %d", err);
        abort();
    }

    err = dispatcher_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "dispatcher_init failed: %d", err);
        abort();
    }

    err = parser_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "parser_init failed: %d", err);
        abort();
    }

    err = transport_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "transport_init failed: %d", err);
        abort();
    }

    ESP_LOGI(TAG, "System initialized");
}
