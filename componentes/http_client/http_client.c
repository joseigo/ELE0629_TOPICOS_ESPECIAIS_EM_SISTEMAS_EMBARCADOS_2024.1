#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"

//Nossa biblioteca
#include "http_client.h"

//Certificado HTTPs para firestore
extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[] asm("_binary_certificate_pem_end");

static const char *TAG = "HTTP CLIENT";

char url_thingspeak[100];
uint8_t temperatura = 25;
uint8_t humidade = 10;

// Semaforo para que a tarefa inicia apenas apos o WiFi conectado (já declarado no wifi.c)
extern SemaphoreHandle_t wifi_semaphore;

void http_client_task(void *pvParameters){

    // Esperamos até uma conexão ser estabelecida
    xSemaphoreTake(wifi_semaphore, portMAX_DELAY);

    while(1){
        ESP_LOGI(TAG, "Fazendo uma requisição HTTPs - http_client_request()!");
        http_client_request();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    
}

//http client handle
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

void http_client_request()
{
//    esp_http_client_config_t config = {
//        .url = "https://firestore.googleapis.com/v1/projects/testebdfirestore/databases/(default)/documents/TesteBD",
//        .method = HTTP_METHOD_GET,
//        .cert_pem = (const char *)certificate_pem_start,
//        .event_handler = _http_event_handle,
//        .transport_type = HTTP_TRANSPORT_OVER_SSL,
//    };

    snprintf(url_thingspeak, sizeof(url_thingspeak), "https://api.thingspeak.com/update?api_key=KBV7FV57GZYLR4N8&field1=%u&field2=%u", temperatura, humidade);
    
    esp_http_client_config_t config = {
        .url = url_thingspeak,
        .method = HTTP_METHOD_GET,
        .cert_pem = (const char *)certificate_pem_start,
        .event_handler = _http_event_handle,
        .skip_cert_common_name_check = true,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t errp = esp_http_client_perform(client);


    if (errp == ESP_OK) {
        ESP_LOGI(TAG, "Status = %i, content_length = %lli", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
    }
    esp_http_client_cleanup(client);

    if (temperatura == 50){
        temperatura = 0;
        humidade = 0;    
    }else{
        temperatura++;
        humidade++;
    }
}