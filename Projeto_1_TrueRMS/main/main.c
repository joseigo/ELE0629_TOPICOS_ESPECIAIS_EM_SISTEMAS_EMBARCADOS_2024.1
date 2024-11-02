#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
// #include "esp_adc/adc_oneshot.h"
// #include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gptimer.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_adc/adc_continuous.h"
#include <string.h>
#include <math.h>
#include "ssd1306.h"

#define BUFFER_SIZE 1024

static const char *TAG1 = "TaskADC";
static const char *TAG2 = "ADC_init";
static const char *TAG3 = "TaskTrueRMS";
static const char *TAG4 = "TaskDisplay";
static const char *TAG5 = "Display_init";

// ---- Display ---- //
SSD1306_t OLED;
int center, top, bottom;
char lineChar[20];

// ---- ADC ---- //
adc_continuous_handle_t adc1_handle = NULL; // Handle ADC1
adc_cali_handle_t cali_adc1_handle = NULL;
// adc_oneshot_unit_handle_t adc1_handle; // Handle ADC1
uint8_t adcBuffer[BUFFER_SIZE] = {0};
uint32_t Num_amostras = 0;

gptimer_handle_t timer = NULL; // Handle Timer
// SemaphoreHandle_t xMutexSemaphore = NULL;
SemaphoreHandle_t xBinarySemaphoreTimer = NULL;
QueueHandle_t xQueueADC = NULL;
QueueHandle_t xQueueRMS = NULL;
QueueHandle_t xQueueTrueRMS = NULL;

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void vTaskADC(void *pvParameter);
void vTaskTrueRMS(void *pvParameter);
void vTaskDisplay(void *pvParameter);

static void IRAM_ATTR timer_isr(void *arg)
{

    // BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    // xSemaphoreGiveFromISR(xBinarySemaphoreTimer, &xHigherPriorityTaskWoken);

    // if (xHigherPriorityTaskWoken == pdTRUE)
    // {
    //     portYIELD_FROM_ISR();
    // }
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    // BaseType_t mustYield = pdFALSE;
    // // Notify that ADC continuous driver has done enough number of conversions
    // vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    // return (mustYield == pdTRUE);

    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    xSemaphoreGiveFromISR(xBinarySemaphoreTimer, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
    return (xHigherPriorityTaskWoken == pdTRUE);
}

void init_timer();
void init_ADC();
void init_display();

void app_main(void)
{
    xQueueADC = xQueueCreate(BUFFER_SIZE, sizeof(float));
    xQueueRMS = xQueueCreate(10, sizeof(float));
    xQueueTrueRMS = xQueueCreate(10, sizeof(float));

    xBinarySemaphoreTimer = xSemaphoreCreateBinary();
    if (xBinarySemaphoreTimer == NULL)
    {
        ESP_LOGI(TAG1, "Fudeu, MAN!");
        xBinarySemaphoreTimer = xSemaphoreCreateBinary();
    }

    init_timer();
    init_ADC();
    init_display();

    // depois implementar a suspensão desta task e resume em uma interupção de botão.

    xTaskCreate(&vTaskADC, TAG1, configMINIMAL_STACK_SIZE * 2, NULL, 4, NULL);
    xTaskCreate(&vTaskTrueRMS, TAG3, configMINIMAL_STACK_SIZE * 2, NULL, 3, NULL);
    xTaskCreate(&vTaskDisplay, TAG4, configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL);
    vTaskDelete(NULL);
}

void vTaskADC(void *pvParameter)

{
    while (1)
    {
        if (xSemaphoreTake(xBinarySemaphoreTimer, portMAX_DELAY) == pdTRUE)
        {

            if (adc_continuous_read(adc1_handle, adcBuffer, BUFFER_SIZE, &Num_amostras, 0) == ESP_OK)
            {
                for (int i = 0; i < Num_amostras; i += SOC_ADC_DIGI_RESULT_BYTES)
                {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t *)&adcBuffer[i];

                    uint32_t adc_raw = p->type1.data;
                    // int voltage_mv = 0;
                    // ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_adc1_handle, adc_raw, &voltage_mv));
                    float voltage_f = map(adc_raw, 0, 4095, -425, 425);
                    // ESP_LOGI(TAG1, "raw: %lu, voltage_mv:%u", adc_raw, voltage_mv);

                    xQueueSend(xQueueADC, &voltage_f, portMAX_DELAY);
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
void vTaskTrueRMS(void *pvParameter)
{
    int NumPoints = BUFFER_SIZE;
    int NumSum = 0;
    float amostra = 0.0;
    float integral = 0.0;
    float Vrms = 0.0;
    float Trms = 0.0;
    float Vmax = -1000;

    while (1)
    {
        while (NumSum < NumPoints)
        {
            if (xQueueReceive(xQueueADC, &amostra, portMAX_DELAY) == pdTRUE)
            {
                // amostra = amostra / 1000; // 1000mv-> 1V
                integral = integral + (amostra) * (amostra);
                if (amostra > Vmax)
                {
                    Vmax = amostra;
                }
                NumSum++;
                // ESP_LOGI(TAG3, "Somas: %u", NumSum);
            }
        }

        integral = integral / NumPoints;

        Trms = sqrtf(integral);

        // Vrms = Vmax / sqrt(2);
        Vrms = 0.636 * Vmax * 1.1111; // fator de forma = 1.1111

        xQueueSend(xQueueTrueRMS, &Trms, portMAX_DELAY);
        xQueueSend(xQueueRMS, &Vrms, portMAX_DELAY);

        // ESP_LOGI(TAG3, "True RMS: %f V, VRMS: %f V", Trms, Vrms);

        NumSum = 0;
        Vmax = -10000;
        integral = 0.0;

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
void vTaskDisplay(void *pvParameter)
{
    float Vrms = 0.0;
    float Trms = 0.0;

    ssd1306_display_text(&OLED, 0, "Multimetro(?)", 14, false);
    char msg[50];
    sprintf(msg, " RMS: %.2f V", Vrms);
    ssd1306_display_text(&OLED, 1, msg, sizeof(msg), false);
    sprintf(msg, "TRMS: %.2f V", Trms);
    ssd1306_display_text(&OLED, 2, msg, sizeof(msg), false);

    while (1)
    {

        if (xQueueReceive(xQueueTrueRMS, &Trms, portMAX_DELAY) == pdTRUE)
        {
            // ESP_LOGI(TAG4, "True RMS: %f V", Trms);
            // sprintf(msg, "TRMS: %.2f V", Trms);
            sprintf(msg, "TRMS: %.2f mV", Trms);
            ssd1306_display_text(&OLED, 2, msg, sizeof(msg), false);
        }
        if (xQueueReceive(xQueueRMS, &Vrms, portMAX_DELAY) == pdTRUE)
        {
            // ESP_LOGI(TAG4, "VRMS: %f V", Vrms);
            // sprintf(msg, " RMS: %.2f V", Vrms);
            sprintf(msg, " RMS: %.2f mV", Vrms);
            ssd1306_display_text(&OLED, 1, msg, sizeof(msg), false);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
void init_timer()
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,                  // counter will reload with 0 on alarm event
        .alarm_count = 15,                  // period = 15us @resolution 1MHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    gptimer_event_callbacks_t isr = {
        .on_alarm = timer_isr, // register user callback
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &isr, NULL));
    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));
}

void init_ADC()
{
    ////////////// CONTINUO DMA ////////////////////
    adc_continuous_handle_cfg_t adc_hconfig = {
        .max_store_buf_size = BUFFER_SIZE * SOC_ADC_DIGI_RESULT_BYTES * 8u,
        .conv_frame_size = 2 * BUFFER_SIZE,
        // .max_store_buf_size = 1024,
        // .conv_frame_size = 256 //
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_hconfig, &adc1_handle));

    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN_DB_0,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
        .channel = ADC_CHANNEL_6, // GPIO34
        .unit = ADC_UNIT_1        //
    };

    adc_continuous_config_t adc_conf = {
        .adc_pattern = &pattern,
        .pattern_num = 1,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .sample_freq_hz = 200000 // fs = 200Khz
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc1_handle, &adc_conf));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc1_handle, &cbs, NULL));

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG2, "Calibração por %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = pattern.unit,
        .atten = pattern.atten,
        .bitwidth = pattern.bit_width,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_adc1_handle));
#endif

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG2, "Calibração por %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = pattern.unit,
        .chan = pattern.channel,
        .atten = pattern.atten,
        .bitwidth = pattern.bit_width,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_adc1_handle));
#endif
    ESP_ERROR_CHECK(adc_continuous_start(adc1_handle));
}
void init_display()
{

    ESP_LOGI(TAG5, "INTERFACE is i2c");
    ESP_LOGI(TAG5, "CONFIG_SDA_GPIO=%d", CONFIG_SDA_GPIO);
    ESP_LOGI(TAG5, "CONFIG_SCL_GPIO=%d", CONFIG_SCL_GPIO);
    ESP_LOGI(TAG5, "CONFIG_RESET_GPIO=%d", CONFIG_RESET_GPIO);
    i2c_master_init(&OLED, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ssd1306_init(&OLED, 128, 32);

    ssd1306_clear_screen(&OLED, false);
    ssd1306_contrast(&OLED, 0xff);
    ssd1306_display_text(&OLED, 0, "  Multimetro(?)", 17, false);
    ssd1306_display_text(&OLED, 1, "Topic. Especiais", 17, false);
    ssd1306_display_text(&OLED, 2, "Sist. Embarcados", 17, false);
    ssd1306_display_text(&OLED, 3, " ELE0629 2024.1 ", 17, false);
    vTaskDelay(4000 / portTICK_PERIOD_MS);

    top = 1;
    center = 1;
    bottom = 4;
    ssd1306_clear_screen(&OLED, false);
    ssd1306_display_text(&OLED, 0, "  Voltimetro!!!", 17, false);
    ssd1306_display_text(&OLED, 1, "De: Bruno e Igo", 17, false);
    ssd1306_display_text(&OLED, 2, "Para: Wallace S2", 17, false);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&OLED, false);
}