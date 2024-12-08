#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "portmacro.h"
#include <rom/ets_sys.h>

#include "config.h"

static char *TAG = "module_USS";

/*
  formoola -> distance to an object = ((speed of sound in the air)*time)/2
              cm = (duration/2) / 29.1
              inches = (duration/2) / 74
*/

static inline double uss_pulseWidthToCM(int64_t width) {
  return (width / 2) / 29.1;
}

void init_USS() {
  ESP_ERROR_CHECK(gpio_reset_pin(TRIG_PIN));
  ESP_ERROR_CHECK(gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_pull_mode(TRIG_PIN, GPIO_PULLDOWN_ENABLE));

  ESP_ERROR_CHECK(gpio_reset_pin(ECHO_PIN));
  ESP_ERROR_CHECK(gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_pull_mode(ECHO_PIN, GPIO_PULLDOWN_ENABLE));

  ESP_ERROR_CHECK(gpio_set_level(TRIG_PIN, 0));

  ESP_LOGI(TAG, "gpio initialized");
}

void uss_measure(int *nextPoll) {

  gpio_set_level(TRIG_PIN, 0);
  vTaskDelay(30 / portTICK_PERIOD_MS);
  gpio_set_level(TRIG_PIN, 1);
  ets_delay_us(US_PING_LEN);
  gpio_set_level(TRIG_PIN, 0);

  int64_t us_start = esp_timer_get_time();
  while (!(gpio_get_level(ECHO_PIN))) {
    if (esp_timer_get_time() > (us_start + (US_TIMEOUT * 1000))) {
      ESP_LOGW(TAG, "TIMEOUT");
      return;
    };
  }
  int64_t us_end = esp_timer_get_time();

  int64_t us_ping_end;
  while (gpio_get_level(ECHO_PIN)) {
  }
  us_ping_end = esp_timer_get_time();

  int64_t pulseWidth = (us_ping_end - us_end);

  *nextPoll = MS_POLLING_RATE - ((us_ping_end - us_start) / 1000);

  // ESP_LOGI(TAG, "RETURN TIME: %lld", (us_end - us_start));
  ESP_LOGI(TAG, "PING_LEN: %lld", pulseWidth);
  ESP_LOGI(TAG, "OBJECT DISTANCE: %lf cm", uss_pulseWidthToCM(pulseWidth));
}

portMUX_TYPE uss_spinlock = portMUX_INITIALIZER_UNLOCKED;

void tsk_ussRead(void *pvParameters) {
  static int nextPoll = MS_POLLING_RATE;

  init_USS();

  for (;;) {
    // taskENTER_CRITICAL(&uss_spinlock);
    uss_measure(&nextPoll);
    // taskEXIT_CRITICAL(&uss_spinlock);
    vTaskDelay(nextPoll / portTICK_PERIOD_MS);
  }
}