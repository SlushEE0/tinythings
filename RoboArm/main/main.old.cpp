/* Blink C++ Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <cstdlib>
#include <functional>
#include <thread>

#include "esp_log.h"
#include "esp_timer_cxx.hpp"
#include "gpio_cxx.hpp"
#include <driver/uart.h>
#include <esp_err.h>

#include "servo.hpp"

using namespace idf;
using namespace std;

extern "C" {
static const char* TAG = "MAIN";

void app_main(void)
{
  try {
    GPIO_Output gpio_3(GPIONum(5));
    GPIOInput gpio_4(GPIONum(4));

    gpio_4.set_pull_mode(GPIOPullMode::PULLDOWN());

    double turns[] = { 0, 45, 90, 135, 180 };
    int current = 0;
    double turnTo = turns[current];

    t_servoCfg cfg = {
      .us_min = 500,
      .us_max = 2500,
      .period = 20000,
      .us_perDeg = 11.111,
      .gpio = &gpio_3,
      .degrees = &turnTo
    };

    for (;;) {
      turnTo = turns[current];
      runCycle(&cfg);

      static bool triggered = false;
      if (gpio_4.get_level() == GPIOLevel::HIGH) {
        ESP_LOGI(TAG, "Btn: HIGH");
        ESP_LOGI(TAG, "Current %d", current);
        if (triggered)
          continue;

        if (current < 4) {
          current += 1;
        } else {
          current = 0;
        }

        triggered = true;
      } else {
        triggered = false;
      }
    }
  } catch (const ESPException& e) {
    ESP_LOGE(TAG, "Error: %d", e.error);
    ESP_LOGE(TAG, "Exiting");
  }
}
}