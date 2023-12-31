#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#define TX_PIN (UART_PIN_NO_CHANGE)
#define RX_PIN (UART_PIN_NO_CHANGE)
#define RTSN_PIN (UART_PIN_NO_CHANGE)
#define CTSN_PIN (UART_PIN_NO_CHANGE)

#define UART_TASK_STACKSIZE (2048)
#define UART_PORT UART_NUM_0
#define UART_BAUD (115200)
#define UART_TXRX_BUFLEN (256)
#define UART_BUF_SIZE (128)

#define PRINTER_READY_MSG "ok"

static const char* TAG = "UART-Reader";
char msgToDevice[ UART_BUF_SIZE ] = "M300 S440 P200";

/*
NOTES::::

* LEARN WHY CHATGPT WORKED

Theories
  - Stack Size
  - All pins are UART_PIN_NO_CHANGE (-1)

Fact
  - Changing the byte size to 1 worked



&data
  13 === Enter

*/

void led()
{
  gpio_set_level(GPIO_NUM_10, 1);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_NUM_10, 0);
}

static uint8_t readUART(char buf[], int maxInputLen)
{
  // Reads the UART console
  // Returns the "KEY" of input
  // Provide a char[] buf to store input

  uint8_t data;
  char* castedByte = " ";

  if (uart_read_bytes(UART_PORT, &data, 1, 10) == 1) {
    castedByte = (char*)&data;

    if (strlen(buf) >= (maxInputLen - 1)) {
      ESP_LOGW(TAG, "Max input length exceeded");

      memset(buf, 0, strlen(buf));
      return data;
    }

    strcat(buf, castedByte);
  }

  return data;
}

static void writeUART(char msg[], char eraseBuf[], int delete)
{
  // writes to the UART console
  // provide msg to write and delete after write
  // 1 == delete after write, 0 == dont delete after write

  char print[ UART_BUF_SIZE ] = "";
  strcpy(print, msg);
  strcat(print, "\n");

  ESP_ERROR_CHECK(uart_flush(UART_PORT));
  uart_write_bytes(UART_PORT, print, strlen(print));
  printf("%s", print);

  if (delete == 1)
    memset(eraseBuf, 0, strlen(eraseBuf));
}

static void uartTask()
{
  uart_config_t uart_config = {
    .baud_rate = UART_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
  ESP_ERROR_CHECK(uart_set_baudrate(UART_PORT, UART_BAUD));
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT, TX_PIN, RX_PIN, RTSN_PIN, CTSN_PIN));
  ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_TXRX_BUFLEN, 0, 0, NULL, 0));

  char buf[ UART_BUF_SIZE ] = "";
  int firstReady = 0;

  while (1) {
    int data = readUART(buf, UART_BUF_SIZE);

    if (strstr(buf, PRINTER_READY_MSG)) {
      led();
      writeUART(msgToDevice, buf, 1);

      if (firstReady == 0)
        firstReady = 1;
    }

    if (firstReady == 0) {
      writeUART(".\r\n", NULL, 0);
    }
  }
}

void app_main(void)
{
  gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT);
  led();

  xTaskCreate(uartTask, "uart_task", UART_TASK_STACKSIZE, NULL, 10, NULL);
}