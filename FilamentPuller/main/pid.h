#include <math.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define PWMPID_TASK_STACKSIZE (4096)

#define PWM_RES_HZ 16384
#define PWM_RES LEDC_TIMER_10_BIT
#define PWM_CONTROL_PIN GPIO_NUM_21;

#define PID_P 1
#define PID_I 0
#define PID_D 0

static const char* TAG_PID = "PID";

/// @param setPoint  Value to reach
/// @param currentTemp  Current value
typedef struct params_pidPwmControlTask_t {
  double* setPoint;
  double* currentTemp;
} params_pidPwmControlTask_t;

/// @param current  Current value
/// @param targetPoint  Value to reach (setPoint)
/// @param clamp  Clamp value from 0 to 1
/// @param prevError  Previous error
/// @param integral  Sum of previous error
typedef struct params_pidController_t {
  double* current;
  double* targetPoint;
  bool clamp;

  double* prevError;
  double* integral;
} params_pidController_t;

void initPWM(bool clamp)
{
  ledc_timer_config_t cfg_pwmTimer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = PWM_RES,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = PWM_RES_HZ,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_channel_config_t cfg_pwmChannel = {
    .gpio_num = 23,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
  };

  ledc_timer_config(&cfg_pwmTimer);
  ledc_channel_config(&cfg_pwmChannel);
}

/// @param percent - Value from 0 to 1
/// @attention percent = 0 => constant LOW
/// @attention percent = 1 => constant HIGH
/// @brief Multiplies max duty cycle by percent.
void updatePWM(double percent)
{
  int dutyCycle = round(percent * (pow(2.0, PWM_RES) - 1));
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyCycle);
}

/// @param current  Current value
/// @param targetPoint  Value to reach (setPoint)
/// @param clamp  Clamp value from 0 to 1
/// @brief PID controller
double pidController(params_pidController_t pidParams)
{
  params_pidController_t* params = &pidParams;

  double* targetPoint = pidParams.targetPoint;
  double* current = pidParams.current;

  double error = targetPoint - current;
  double* prevError = params->prevError;
  double* integral = params->integral;

  *integral += error;

  double pOut = PID_P * error;
  double iOut = PID_I * *integral;
  double dOut = PID_D * (error - *prevError);

  *prevError = error;

  double pidOut = (pOut + iOut + dOut) / *targetPoint;
  ESP_LOGI(TAG_PID, "pOut: %f", pidOut);

  if (!params->clamp)
    return pidOut;

  if (pidOut > 1) {
    return 1;
  } else if (pidOut < 0) {
    return 0;
  } else {
    return pidOut;
  }
}

void task_pidPwmControl(void* pvParameters)
{
  params_pidPwmControlTask_t* params = (params_pidPwmControlTask_t*)pvParameters;

  double* setPoint = params->setPoint;
  double* currentTemp = params->currentTemp;

  double* prevError = 0;
  double* integral = 0;

  params_pidController_t pidParams = {
    .current = currentTemp,
    .targetPoint = setPoint,
    .clamp = true,

    .prevError = prevError,
    .integral = integral
  };

  while (1) {
    double pidOut = pidController(pidParams);

    ESP_LOGI(TAG_PID, "pidOut: %f", pidOut);
    ESP_LOGI(TAG_PID, "setPoint: %f", *setPoint);
    ESP_LOGI(TAG_PID, "currentTemp: %f", *currentTemp);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}