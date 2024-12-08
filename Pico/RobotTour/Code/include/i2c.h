#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/binary_info.h"

#include "constants.h"

static i2c_inst_t *INTERFACE;

void I2C_init(i2c_inst_t *i2c, uint baudrate) {
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

  INTERFACE = i2c;
  i2c_init(i2c, baudrate);
}

void I2C_scanbus() {
  printf("\nI2C Bus Scan\n");
  printf(" 0 1 2 3 4 5 6 7 8 9 A B C D E F\n");

  for (int addr = 0; addr < (1 << 7); ++addr) {
    if (addr % 16 == 0) {
    }
  }
}

int I2C_read(uint8_t addr, uint8_t *rxdata) {
  return i2c_read_blocking(INTERFACE, addr, rxdata, 1, false);
}