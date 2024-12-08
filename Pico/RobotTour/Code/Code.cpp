#include "hardware/timer.h"
#include "include/constants.h"
#include "include/control.h"
#include "pico/stdlib.h"
#include <hardware/clocks.h>
#include <stdio.h>
#include <string.h>

int main() {
  stdio_init_all();
  gpio_set_dir(25, true);
  gpio_put(25, 1);

  // add_alarm_in_ms(2000, alarm_callback, NULL, false);
  sleep_ms(3000);

  printf("USB CONNECTED\n");
  printf("System Clock Frequency is %d Hz\n", clock_get_hz(clk_sys));
  printf("USB Clock Frequency is %d Hz\n", clock_get_hz(clk_usb));
  /* -----------------------INITIALIZE----------------------- */

  control_init();

  while (true) {
    control_mainLoop();
  };
}
