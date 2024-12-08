#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "portmacro.h"
#include <esp_err.h>
#include <rom/ets_sys.h>

#include "config.h"
#include "uss.h"

static char *TAG = "module_USS";

static inline double uss_pulseWidthToCM(int64_t width) {
  return (width / 2) / 29.1;
}

void init_GPIO() {
  ESP_ERROR_CHECK(gpio_reset_pin(CONFIG_TRIG_PIN));
  ESP_ERROR_CHECK(gpio_set_direction(CONFIG_TRIG_PIN, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_pull_mode(CONFIG_TRIG_PIN, GPIO_PULLDOWN_ENABLE));

  ESP_ERROR_CHECK(gpio_reset_pin(CONFIG_ECHO_PIN));
  ESP_ERROR_CHECK(gpio_set_direction(CONFIG_ECHO_PIN, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_pull_mode(CONFIG_ECHO_PIN, GPIO_PULLDOWN_ENABLE));

  ESP_ERROR_CHECK(gpio_set_level(CONFIG_TRIG_PIN, 0));

  ESP_LOGI(TAG, "gpio initialized");
}

portMUX_TYPE uss_spinlock = portMUX_INITIALIZER_UNLOCKED;

double uss_measure() {
  taskENTER_CRITICAL(&uss_spinlock);

  gpio_set_level(CONFIG_TRIG_PIN, 1);
  ets_delay_us(CONFIG_US_PING_LEN);
  gpio_set_level(CONFIG_TRIG_PIN, 0);

  int64_t us_start = esp_timer_get_time();
  while (!(gpio_get_level(CONFIG_ECHO_PIN))) {
    if (esp_timer_get_time() > (us_start + (CONFIG_US_TIMEOUT * 1000))) {
      ESP_LOGW(TAG, "TIMEOUT");
      return -1;
    };
  }
  int64_t us_end = esp_timer_get_time();

  int64_t us_ping_end;
  while (1) {
    if (gpio_get_level(CONFIG_ECHO_PIN)) continue;
    ets_delay_us(3);
    if (gpio_get_level(CONFIG_ECHO_PIN)) continue;

    break;
  }
  us_ping_end = esp_timer_get_time();

  taskEXIT_CRITICAL(&uss_spinlock);

  int64_t pulseWidth = (us_ping_end - us_end);
  double cm_distance = uss_pulseWidthToCM(pulseWidth);

  // ESP_LOGI(TAG, "RETURN TIME: %lld", (us_end - us_start));
  ESP_LOGI(TAG, "OBJECT DISTANCE: %lf cm", cm_distance);

  return cm_distance;
}

bool garage_getIsOpen() {
  double distance = uss_measure();

  if (distance == -1) {
    ESP_LOGE(TAG, "ERROR TIMEOUT");
    return false;
  }

  return distance <= CONFIG_GARAGE_DISTANCE_CM;
}