#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "hardware/timer.h"

#include "constants.h"

void SPI_init() {
  uint baud = spi_init(SPI_INTERFACE, SPI_HZ);
  spi_set_slave(SPI_INTERFACE, false);
  spi_set_format(SPI_INTERFACE, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

  printf("Baudrate: %u\n", baud);

  gpio_set_function(MPU_CLK, GPIO_FUNC_SPI);
  gpio_set_function(MPU_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(MPU_MISO, GPIO_FUNC_SPI);

  bi_decl(bi_3pins_with_func(MPU_MISO, MPU_MOSI, MPU_CLK, GPIO_FUNC_SPI));

  // CS active low so drive it high
  gpio_init(MPU_CS);
  gpio_set_dir(MPU_CS, GPIO_OUT);
  gpio_put(MPU_CS, 1);

  bi_decl(bi_1pin_with_name(MPU_CS, "SPI CS"));
}

static inline void SPI_start() {
  asm volatile("nop \n nop \n nop");
  gpio_put(MPU_CS, 0); // Active low
  asm volatile("nop \n nop \n nop");
}

static inline void SPI_end() {
  asm volatile("nop \n nop \n nop");
  gpio_put(MPU_CS, 1);
  asm volatile("nop \n nop \n nop");
}

int SPI_write(uint8_t reg, uint8_t data) {
  reg |= 0x80;

  SPI_start();
  spi_write_blocking(SPI_INTERFACE, &reg, 8); // Send register address
  sleep_ms(5);
  int bytes = spi_write_blocking(SPI_INTERFACE, &data, 8); // Write the data
  SPI_end();

  return bytes;
}

int SPI_read(uint8_t reg, uint8_t *buf, uint16_t len) {
  uint8_t readReg = reg | 0x80;

  SPI_start();
  sleep_us(10);
  spi_write_blocking(SPI_INTERFACE, &readReg, 1); // Send register address
  int bytes =
    spi_read_blocking(SPI_INTERFACE, 0, buf, 1); // Read the response
  SPI_end();

  printf("read %d bytes from %02X\n", bytes, reg);

  return bytes;
}
