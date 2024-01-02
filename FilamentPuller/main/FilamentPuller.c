
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "pid.h"
#include "uart.h"

static const char* TAG = "MAIN";

static double currentTemp = 2;
static double targetTemp = 0;

void taskFunc_UART(char buf[], int data)
{
  switch (data) {
  case 13:
    // writeUART(buf, NULL, 0);
    targetTemp += 1;
    ESP_LOGI(TAG, "changed setpoint %f", targetTemp);
    break;

  case 93:
    currentTemp += 1;
    ESP_LOGI(TAG, "changed current %f", currentTemp);
    break;

  default:
    // ESP_LOGI(TAG, "unknown command: %d", data);
    break;
  }
}

void app_main(void)
{
  params_UartTask_t uartTaskParams = {
    .tag = "UART",
    .taskFunc = taskFunc_UART,
  };
  params_pidPwmControlTask_t pidTaskParams = {
    .currentTemp = &currentTemp,
    .setPoint = &targetTemp,
  };

  xTaskCreate(task_pidPwmControl, "control temp", PWMPID_TASK_STACKSIZE, &pidTaskParams, 10, NULL);
  xTaskCreate(task_UART, "uart_task", UART_TASK_STACKSIZE, &uartTaskParams, 10, NULL);
}
