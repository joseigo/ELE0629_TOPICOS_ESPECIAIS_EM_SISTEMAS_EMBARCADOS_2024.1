
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

bool lastButton1State = true;
bool lastButton2State = true;

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
}

void app_main(void)
{
    gpio_init();
    int cont = 0;
    while (1)
    {

        bool button1State = gpio_get_level(BUTTON1_PIN);
        if (button1State != lastButton1State)
        {
            lastButton1State = button1State;
            if (!button1State)
            {
                LED2State = !LED2State;
            }
        }

        bool button2State = gpio_get_level(BUTTON2_PIN);
        if (button2State != lastButton2State)
        {
            lastButton2State = button2State;
            if (!button2State)
            {
                LED3State = !LED3State;
            }
        }

        gpio_set_level(LED1_PIN, LED1State);
        gpio_set_level(LED2_PIN, LED2State);
        gpio_set_level(LED3_PIN, LED3State);

        ESP_LOGI(TAG, "\nLED1 %s!"
                      "\nLED2 %s!"
                      "\nLED3 %s!",
                 LED1State == true ? "ON" : "OFF", LED2State == true ? "ON" : "OFF", LED3State == true ? "ON" : "OFF");

        vTaskDelay(50 / portTICK_PERIOD_MS);
        if ((cont++) > 10)
        {
            LED1State = !LED1State;
            cont = 0;
        }
    }
}