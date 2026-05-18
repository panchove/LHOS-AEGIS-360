#include "dispatcher.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "dispatcher";

#define DISPATCHER_TASK_STACK 4096
#define DISPATCHER_TASK_PRIO 7
#define DISPATCH_QUEUE_LEN 32

static QueueHandle_t s_dispatch_q = NULL;

static void dispatcher_task(void *arg)
{
    packet_buf_t* p;
    for (;;) {
        if (xQueueReceive(s_dispatch_q, &p, portMAX_DELAY) == pdTRUE) {
            // Example consumer: process and release
            ESP_LOGI(TAG, "Dispatching packet len=%d", p->len);
            // TODO: route to real service(s)
            // Simulate processing
            vTaskDelay(pdMS_TO_TICKS(10));
            // Release ownership when consumer done
            pool_release(p);
        }
    }
}

esp_err_t dispatcher_init(void)
{
    if (s_dispatch_q == NULL) {
        s_dispatch_q = xQueueCreate(DISPATCH_QUEUE_LEN, sizeof(packet_buf_t*));
        if (s_dispatch_q == NULL) return ESP_ERR_NO_MEM;
    }
    BaseType_t rc = xTaskCreatePinnedToCore(dispatcher_task, "dispatcher", DISPATCHER_TASK_STACK, NULL, DISPATCHER_TASK_PRIO, NULL, 1);
    if (rc != pdPASS) return ESP_ERR_NO_MEM;
    ESP_LOGI(TAG, "dispatcher initialized");
    return ESP_OK;
}

esp_err_t dispatcher_submit(packet_buf_t* packet)
{
    if (packet == NULL) return ESP_ERR_INVALID_ARG;
    if (s_dispatch_q == NULL) return ESP_ERR_INVALID_STATE;
    if (xQueueSend(s_dispatch_q, &packet, pdMS_TO_TICKS(50)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}
