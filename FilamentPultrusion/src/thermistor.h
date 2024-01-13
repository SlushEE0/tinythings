#include <math.h>
#include <stdio.h>
#include <string.h>

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#ifndef BIT_WIDTH
#define BIT_WIDTH (ADC_WIDTH_BIT_12)
#endif

#ifndef ADC_PIN
#define ADC_PIN GPIO_NUM_23
#endif

#define RESISTOR_VAL 5000

static char* TAG_THERM = "TEMP";

void init_adc()
{
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
}

double getTemperature()
{
  uint32_t adc_value = adc1_get_raw(ADC1_CHANNEL_0);

  double voltage = (adc_value / (pow(2.0, BIT_WIDTH) - 1)) * 3.3;
  double resistance = (3.3 - voltage) / (voltage / RESISTOR_VAL);
  double temperature = resistance;
}

void task_temp()
{
  init_adc();

  while (1) {
    double temperature = getTemperature();

    ESP_LOGI(TAG_THERM, "Temperature: %.2f°C", temperature);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}