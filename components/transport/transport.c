#include "transport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "parser.h"

static const char *TAG = "transport";

#define UART_NUM_USED UART_NUM_1
#define UART_RX_BUF_SIZE 2048
#define UART_EVENT_QUEUE_LEN 16

static QueueHandle_t uart_queue = NULL;

static void uart_event_task(void *arg)
{
    uart_event_t event;
    uint8_t local_buf[256];
    for (;;) {
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int len = uart_read_bytes(UART_NUM_USED, local_buf, sizeof(local_buf), pdMS_TO_TICKS(10));
                if (len > 0) {
                    parser_feed(local_buf, len);
                }
            }
        }
    }
}

esp_err_t transport_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    esp_err_t err = uart_param_config(UART_NUM_USED, &uart_config);
    if (err != ESP_OK) return err;
    // pins left default; user must configure if needed
    err = uart_driver_install(UART_NUM_USED, UART_RX_BUF_SIZE, 0, UART_EVENT_QUEUE_LEN, &uart_queue, 0);
    if (err != ESP_OK) return err;

    // Create event task pinned to core 1 (processing core)
    BaseType_t rc = xTaskCreatePinnedToCore(uart_event_task, "uart_event", 3072, NULL, configMAX_PRIORITIES - 2, NULL, 1);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create uart_event_task");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "transport initialized on UART%d", UART_NUM_USED);
    return ESP_OK;
}
