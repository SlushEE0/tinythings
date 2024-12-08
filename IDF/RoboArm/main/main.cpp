#include <cstdlib>
#include <functional>
#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer_cxx.hpp"
#include <driver/uart.h>
#include <esp_err.h>

#include "gpio_cxx.hpp"

#define MS_STEP_DELAY (500)

using namespace idf;
using namespace std;

static const char* TAG = "MAIN";

void step(GPIO_Output* a, GPIO_Output* b)
{
  a->set_high();
  b->set_high();
  vTaskDelay(MS_STEP_DELAY / portTICK_PERIOD_MS);
  a->set_low();
  b->set_low();
}

void task_Stepper(void* params)
{
}

extern "C" void app_main()
{
  GPIO_Output gpio_1(GPIONum(3));  // blue
  GPIO_Output gpio_2(GPIONum(2));  // pink
  GPIO_Output gpio_3(GPIONum(11)); // yellow
  GPIO_Output gpio_4(GPIONum(10)); // orange

  GPIOInput gpio_button(GPIONum(4));
  gpio_button.set_pull_mode(GPIOPullMode::PULLDOWN());

  while (1) {
    step(&gpio_1, &gpio_2);
    step(&gpio_2, &gpio_3);
    step(&gpio_3, &gpio_4);
    step(&gpio_4, &gpio_1);
  }
}
