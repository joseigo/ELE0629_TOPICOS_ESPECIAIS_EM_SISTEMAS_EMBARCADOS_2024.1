#include <stdio.h>
#include <string.h>

// Includes FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

// Includes ESP-IDF
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Include HTTP
#include "esp_http_client.h"

// Biblioteca criada
#include "wifi.h"
#include "http_client.h"

static const char *TAG = "Exemplo Wi-Fi";

void app_main(void)
{
    // Inicializando NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // Task para fazer as requisições http
    xTaskCreate(&http_client_task, "http_client_task", 4096, NULL, 5, NULL);
}
