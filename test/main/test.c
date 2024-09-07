#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char* TAG = "Main";

void task_logger() {
  ESP_LOGI(TAG, "Initalized");

  for (;;)
  {
    ESP_LOGI(TAG, "Initalized");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  
}

void app_main(void)
{
  ESP_LOGI(TAG, "HELLO");
  xTaskCreate(&task_logger, "Logger", 2048, NULL, 3, NULL);
}