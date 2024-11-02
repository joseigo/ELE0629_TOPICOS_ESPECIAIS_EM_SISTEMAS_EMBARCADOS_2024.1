#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "esp_mac.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 2

const char *TAG = "receiver ESP-NOW";

void espnow_receiver_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_length){
    ESP_LOGI(TAG, "Recebendo mensagem de: "MACSTR, MAC2STR(esp_now_info->src_addr));
    printf("Mensagem: %.*s\n", data_length, data);

    if(strncmp((char *)data, "Led ON", data_length) == 0){
        gpio_set_level(LED_PIN, 1);
    }else if(strncmp((char *)data, "Led OFF", data_length) == 0){
        gpio_set_level(LED_PIN, 0);
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Inicializando o Wi-Fi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    //Inicialização do ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_receiver_cb));   //Função de callback para tratar o evento de envio

    // Não é preciso adicionar o peer, pois o sender inicia a comunicação

    // Configurando o pino led
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}