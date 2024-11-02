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
#include "esp_mac.h"

// Include LwIP
#include "lwip/err.h"
#include "lwip/sys.h"

// Nossa Biblioteca
#include "wifi.h"

// // Credenciais do Wi-Fi definidas no menuconfig
// #define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
// #define EXAMPLE_ESP_WIFI_PASSWORD CONFIG_ESP_WIFI_PASSWORD
// #define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

// Credenciais do Wi-Fi casa
#define EXAMPLE_ESP_WIFI_SSID "brisa-2138502"
#define EXAMPLE_ESP_WIFI_PASSWORD "sapij9f7"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

// Definição dos bits para o grupo de eventos (BIT0 e BIT1 já são macros, não precisamos realizar deslocamento de bits)
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Semaforo para liberar a requisição http somente quando houver uma conexão
SemaphoreHandle_t wifi_semaphore;

// Sincronizar as tarefas conforme os eventos WiFi ocorrem
static EventGroupHandle_t s_wifi_event_group;

// TAG para info
static const char *TAG = "Biblioteca Wi-Fi";

// Quantidade de tentativas para conexão
static int s_retry_num = 0;

// Função para tratamento dos eventos
static void sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    // Verificar se o evento gerado é Wi-Fi e qual o motivo
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {

        // Tentando reconectar
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Tentando se conectar %d", s_retry_num);
        }
        else
        {
            // Caso não consiga conexão indica ao grupo de eventos
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Falha na conexão");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {

        // Verificando se o evento é referente ao IP
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Pegamos um IP " IPSTR, IP2STR(&event->ip_info.ip));

        s_retry_num = 0;

        // Depois de ter conseguido IP a conexão foi realizada com sucesso. Indica ao grupo de eventos
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Função para tratamento dos eventos
static void ap_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    // Verificar se o evento gerado é Wi-Fi e qual o motivo
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "MAC do Station: " MACSTR " CONECTADO, AID = %d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "MAC do Station: " MACSTR " DESCONECTADO, AID = %d", MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_sta(void)
{
    wifi_semaphore = xSemaphoreCreateBinary();
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());                // LwIP
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Event loop for event task
    esp_netif_create_default_wifi_sta();              // Cria LwIP para config station

    // Configurando o Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Manipulador de eventos WiFi
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL));

    // Configuração da estrutura do WiFi modo sta com as credenciais e autenticações
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    // Definição do modo station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // Configuração do Wifi a partir da estrutura previamente definida
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // Start no WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Configuração e inicialização do Wi-Fi finalizado!!!");

    // Esperando os eventos do Wi-Fi para seguir com tratamento
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "CONECTADO A SSID: %s password: %s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASSWORD);
        // Conexão estabelecida, liberamos o semaforo para a proxima tarefa que precise de conexão
        xSemaphoreGive(wifi_semaphore);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "FALHA AO SE CONECTAR A SSID: %s password: %s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASSWORD);
    }
    else
    {
        ESP_LOGI(TAG, "EVENTO INESPERADO!!!");
    }
}

void wifi_init_ap(const char *ssid, const char *pass)
{
    // Init Phase - Wi-Fi Driver and LwIP
    ESP_ERROR_CHECK(esp_netif_init());                   // LwIP
    ESP_ERROR_CHECK(esp_event_loop_create_default());    // Event loop for event task
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Driver Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Realizando o storage na RAM. Deixa a conexão um pouco mais rápida.
    // É uma outra forma de configurar, ao invés de modo persistente na NVS.
    // Obs: Perdendo a conexão é preciso realizar a configurações novamente (executa essa mesma rotina).
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_LOGI(TAG, "INICIALIZANDO O WI-FI!");
    esp_netif_create_default_wifi_ap(); // LwIP com configurações básicas para o modo AP.

    // Wi-Fi Configuration Phase
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    strncpy((char *)wifi_config.ap.password, pass, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = 5;
    wifi_config.ap.beacon_interval = 100; // Deve ser múltiplo de 100. Faixa de 100 a 600000. Unit: TU(time unit, 1 TU = 1024 us).
    wifi_config.ap.channel = 1;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Wi-Fi start Phase
    esp_wifi_start();

    ESP_LOGI(TAG, "ESP32 COMO AP PRONTO! CONECTAR AO SSID: %s, PASSWORD: %s E CANAL: %d",
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);
}