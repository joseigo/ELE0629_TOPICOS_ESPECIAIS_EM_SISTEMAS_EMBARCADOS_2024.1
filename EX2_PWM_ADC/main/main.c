///////////////////////////////////////////////////////////////////////
////// ELE0629 - TÓPICOS ESPECIAIS EM SISTEMAS EMBARCADOS 2024.1 //////
///////////////////////////////////////////////////////////////////////

/* Exercício 2:

    • Ler o valor de tensão de um potenciômetro com o ADC;
    • Utilizar esse valor para atualizar o duty cycle de um sinal PWM que acenderá um led;
    • Criar uma TAG para mostrar o valor atualizado do duty cycle;

*/

#include <stdio.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "EX2_PWM_ADC";
adc_oneshot_unit_handle_t adc1_handle; // Handle ADC1

void PWM_init()
{
    // Configurando a estrutura para o timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .clk_cfg = LEDC_APB_CLK};
    ledc_timer_config(&timer_conf); // Aplica a conf ao Timer

    // Configurando o canal a ser utilizado
    ledc_channel_config_t channel_conf = {
        .channel = LEDC_CHANNEL_1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_NUM_2, // LED do ESP
        .duty = 0};
    ledc_channel_config(&channel_conf);
}

void ADC_init()
{
    // Inicializando o ADC. Define qual ADC será utilizado:
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configurando o ADC (canal a ser utilizado):
    adc_oneshot_chan_cfg_t channel_conf = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11};
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &channel_conf));
}

int getADCraw()
{
    int adc_raw = 0;
    if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &adc_raw) == ESP_OK)
    {
        return ((adc_raw * 1024) / 4095);
    }
    return -1;
}

void app_main(void)
{
    PWM_init();
    ADC_init();

    int duty = 0;
    int raw = 0;
    while (1)
    {
        ESP_LOGI(TAG, "Duty %u!", duty);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, (uint32_t)duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        raw = getADCraw();
        if (raw != -1)
        {
            duty = raw;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
