#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"

// Nossa biblioteca
#include "http_client.h"

// Certificado HTTP
extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[] asm("_binary_certificate_pem_end");

// Certificado HTTPs para firestore

static const char *TAG = "HTTP CLIENT";

static char response_buffer[1024] = {0};

// Semaforo para que a tarefa inicia apenas apos o WiFi conectado (já declarado no wifi.c)
extern SemaphoreHandle_t wifi_semaphore;

static void send_to_firebase();

void http_client_task(void *pvParameters)
{

    // Esperamos até uma conexão ser estabelecida
    xSemaphoreTake(wifi_semaphore, portMAX_DELAY);

    while (1)
    {
        ESP_LOGI(TAG, "Fazendo uma requisição HTTPs - http_client_request()!");
        http_client_request();
        send_to_firebase();
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

// http client handle
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
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
        printf("%.*s", evt->data_len, (char *)evt->data);
        if (evt->data_len + strlen(response_buffer) < sizeof(response_buffer))
        {
            strncat(response_buffer, (char *)evt->data, evt->data_len);
        }
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

    // snprintf(url_thingspeak, sizeof(url_thingspeak), "https://api.thingspeak.com/update?api_key=KBV7FV57GZYLR4N8&field1=%u&field2=%u", temperatura, humidade);

    // esp_http_client_config_t config = {
    //     .url = url_thingspeak,
    //     .method = HTTP_METHOD_GET,
    //     .cert_pem = (const char *)certificate_pem_start,
    //     .event_handler = _http_event_handle,
    //     .skip_cert_common_name_check = true,
    //     .transport_type = HTTP_TRANSPORT_OVER_SSL,
    // };
    esp_http_client_config_t config = {
        .url = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=brl",
        .method = HTTP_METHOD_GET,
        .cert_pem = (const char *)certificate_pem_start,
        .event_handler = _http_event_handle,
        .skip_cert_common_name_check = false,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t errp = esp_http_client_perform(client);

    if (errp == ESP_OK)
    {
        ESP_LOGI(TAG, "Status = %i, content_length = %lli", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(errp));
    }
    esp_http_client_cleanup(client);
}

static void send_to_firebase()
{
    esp_http_client_config_t config = {
        .url = "https://firestore.googleapis.com/v1/projects/atv5-tese2/databases/(default)/documents/moeda/bitcoin",
        .method = HTTP_METHOD_PATCH,
        .cert_pem = (const char *)certificate_pem_start,
        .event_handler = _http_event_handle,
        .timeout_ms = 2000,
        .buffer_size = 1024,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    const char *bitcoin_start_str = "\"brl\":";
    char bitcoin_value[16] = {0};
    char *bitcoin_start = strstr(response_buffer, bitcoin_start_str);
    if (bitcoin_start != NULL)
    {
        char *bitcoin_end = strchr(bitcoin_start, '}');
        if (bitcoin_end != NULL)
        {
            bitcoin_start += strlen(bitcoin_start_str);
            size_t bitcoin_len = bitcoin_end - bitcoin_start;
            strncpy(bitcoin_value, bitcoin_start, bitcoin_len);
            bitcoin_value[bitcoin_len] = '\0';
        }
    }

    char post_data[64];
    sprintf(post_data, "{\"fields\":{\"valor\":{\"stringValue\":\"%s\"}}}", bitcoin_value);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Dados enviados com sucesso para o Firebase!");
    }
    else
    {
        ESP_LOGE(TAG, "Falha ao enviar dados para o Firebase");
    }
    esp_http_client_cleanup(client);
    memset(response_buffer, 0, sizeof(response_buffer));
}