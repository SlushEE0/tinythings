#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "spi.h"

void MPU_init() {
  SPI_init();

  uint8_t* data_whois;
  int bytes = SPI_read(0x75, data_whois, sizeof(data_whois));

  printf("Whois: %02X\n", *data_whois);
}