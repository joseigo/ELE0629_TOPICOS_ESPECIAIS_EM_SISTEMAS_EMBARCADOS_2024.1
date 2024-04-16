#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>

#define BUFFER_SIZE 10

static const char *TAG1 = "Temperatura";
static const char *TAG2 = "Umidade";
static const char *TAG3 = "Velocidade";
static const char *TAG4 = "Peso";
static const char *TAG5 = "Distancia";

static const char *TAG6 = "TaskLeitura1";
static const char *TAG7 = "TaskLeitura2";

char buffer[BUFFER_SIZE][13];

SemaphoreHandle_t xMutexSemaphore = NULL;

void vTaskGetData(void *pvParameter);
void vTaskSetData(void *pvParameter);

void app_main(void)
{
    xMutexSemaphore = xSemaphoreCreateMutex();

    xTaskCreate(vTaskSetData, TAG1, 2048, (void *)TAG1, 1, NULL);
    xTaskCreate(vTaskSetData, TAG2, 2048, (void *)TAG2, 1, NULL);
    xTaskCreate(vTaskSetData, TAG3, 2048, (void *)TAG3, 1, NULL);
    xTaskCreate(vTaskSetData, TAG4, 2048, (void *)TAG4, 1, NULL);
    xTaskCreate(vTaskSetData, TAG5, 2048, (void *)TAG5, 1, NULL);

    xTaskCreate(vTaskGetData, TAG6, 2048, (void *)TAG6, 1, NULL);
    xTaskCreate(vTaskGetData, TAG7, 2048, (void *)TAG7, 1, NULL);
}

void vTaskGetData(void *pvParameter)
{
    const char *LOG_task = (const char *)pvParameter;

    while (1)
    {
        if (xSemaphoreTake(xMutexSemaphore, portMAX_DELAY) == pdTRUE)
        {
            int position = -1;
            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                if (buffer[i][0] != '\0')
                {
                    position = i;
                    break;
                }
            }

            if (position != -1)
            {
                ESP_LOGI(LOG_task, "%s", buffer[position]);
                buffer[position][0] = '\0';
            }
            else
            {
                ESP_LOGI(LOG_task, "Leitura finalizada!");
                xSemaphoreGive(xMutexSemaphore);
                vTaskDelete(NULL);
            }

            xSemaphoreGive(xMutexSemaphore);
        }
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
}

void vTaskSetData(void *pvParameter)
{
    const char *LOG_task = (const char *)pvParameter;
    int cont_white = 0;

    while (cont_white < 5)
    {
        if (xSemaphoreTake(xMutexSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Procura por uma posição vazia no buffer
            int position = -1;
            for (int j = 0; j < BUFFER_SIZE; j++)
            {
                if (buffer[j][0] == '\0')
                {
                    position = j;
                    break;
                }
            }

            if (position != -1)
            {
                // Escreve o nome da tarefa no buffer
                sprintf(buffer[position], "%s", LOG_task);
                // ESP_LOGI(LOG_task, "Escreveu!");
                cont_white++;
            }
            // else
            // {
            //     xSemaphoreGive(xMutexSemaphore);
            //     xSemaphoreTake(xMutexSemaphore, pdMS_TO_TICKS(200));
            // }

            xSemaphoreGive(xMutexSemaphore); // Libera o mutex após a escrita
        }

        vTaskDelay(25 / portTICK_PERIOD_MS); // Intervalo entre escritas
    }

    // A tarefa se deleta após completar as escritas
    ESP_LOGI(LOG_task, "Escrita finalizada!");
    vTaskDelete(xTaskGetCurrentTaskHandle());
}
