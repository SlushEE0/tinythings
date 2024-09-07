#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "uss.h"

#include "config.h"

#define TAG ("ESP")


void app_main(void) {
  xTaskCreate(&tsk_ussRead, "Reader Task", 2048, NULL, portPRIVILEGE_BIT, NULL);
}
