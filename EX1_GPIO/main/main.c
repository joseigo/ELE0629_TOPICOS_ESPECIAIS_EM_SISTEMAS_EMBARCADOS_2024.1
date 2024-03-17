///////////////////////////////////////////////////////////////////////
////// ELE0629 - TÓPICOS ESPECIAIS EM SISTEMAS EMBARCADOS 2024.1 //////
///////////////////////////////////////////////////////////////////////

/* Exercício 1:

    • Alternar o modo do LED1 (aceso ou apagado) sempre que pressionar um botão1;
    • Alternar o modo do LED2 (aceso ou apagado) sempre que pressionar um botão2;
    • Acender o LED3 com uma frequência de 1 segundos;
    • Criar uma TAG para informar o estado de cada LED, por meio de um LOGI;

*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define LED1_PIN GPIO_NUM_2
#define LED2_PIN GPIO_NUM_23
#define LED3_PIN GPIO_NUM_22
#define BUTTON1_PIN GPIO_NUM_18
#define BUTTON2_PIN GPIO_NUM_19

static const char *TAG = "EX1_GPIO";

bool LED1State = false;
bool LED2State = false;
bool LED3State = false;

void IRAM_ATTR button1_isr(void *args)
{
    gpio_set_level(LED2_PIN, !gpio_get_level(LED2_PIN));
}
void IRAM_ATTR button2_isr(void *args)
{
    gpio_set_level(LED3_PIN, !gpio_get_level(LED3_PIN));
}

void gpio_init()
{
    // Configurando os pinos dos LEDs;
    gpio_reset_pin(LED1_PIN);
    gpio_set_direction(LED1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED1_PIN, 0);
    gpio_reset_pin(LED2_PIN);
    gpio_set_direction(LED2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED2_PIN, 0);
    gpio_reset_pin(LED3_PIN);
    gpio_set_direction(LED3_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED3_PIN, 0);

    // Configurando os pinos dos botôes;
    gpio_reset_pin(BUTTON1_PIN);
    gpio_set_direction(BUTTON1_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON1_PIN, GPIO_PULLUP_ONLY);
    gpio_reset_pin(BUTTON2_PIN);
    gpio_set_direction(BUTTON2_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON2_PIN, GPIO_PULLUP_ONLY);

    // Habilitando as interrupções dos botões;
    gpio_set_intr_type(BUTTON1_PIN, GPIO_INTR_LOW_LEVEL);
    gpio_set_intr_type(BUTTON2_PIN, GPIO_INTR_LOW_LEVEL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON1_PIN, button1_isr, NULL);
    gpio_isr_handler_add(BUTTON2_PIN, button2_isr, NULL);
}

void app_main(void)
{
    gpio_init();
    while (1)
    {
        ESP_LOGI(TAG, "LED1 %s!", LED1State == true ? "ON" : "OFF");
        ESP_LOGI(TAG, "LED2 %s!", LED2State == true ? "ON" : "OFF");
        ESP_LOGI(TAG, "LED3 %s!", LED3State == true ? "ON" : "OFF");
        gpio_set_level(LED1_PIN, LED1State);
        LED1State = !LED1State;
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}