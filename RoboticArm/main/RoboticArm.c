#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Servo.h"

uint32_t servo1angle = 0;
uint32_t servo2angle = 0;

void app_main(void)
{
  xTaskCreate(task_updateServoAngle, "ServoController", (void*)&servo1angle, NULL, 3, NULL);
}
