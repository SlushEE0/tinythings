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
#define UART_PORT (UART_NUM_0)
#define UART_BAUD (115200)
#define UART_TXRX_BUFLEN (256)
#define UART_BUF_SIZE (128)

static char* TAG_UART = "UART";

/*
NOTES::::

*/

typedef struct params_UartTask_t {
  void (*taskFunc)(char buf[], int data);
  char* tag;
} params_UartTask_t;

void writeUART(char msg[], char eraseBuf[], int delete)
{
  // writes to the UART console
  // provide msg to write and delete after write
  // 1 == delete after write, 0 == dont delete after write

  char* print = (char*)malloc(sizeof(char) * UART_BUF_SIZE);
  strcpy(print, msg);
  strcat(print, "\n");

  ESP_ERROR_CHECK(uart_flush(UART_PORT));
  uart_write_bytes(UART_PORT, print, strlen(print));
  ESP_ERROR_CHECK(uart_flush(UART_PORT));
  
  free(print);

  if (delete == 1)
    memset(eraseBuf, 0, strlen(eraseBuf));
}

int readUART(char buf[], int maxInputLen)
{
  // Reads the UART console
  // Returns the Keyboard ID of input
  // Provide a char[] buf to store input

  uint8_t data;
  char* castedByte = " ";

  if (strlen(buf) >= (maxInputLen - 1)) {
    ESP_LOGI(TAG_UART, "Buffer Overflow");
    writeUART(buf, buf, true);
  } else if (uart_read_bytes(UART_PORT, &data, 1, 10) == 1) {
    castedByte = (char*)&data;
    strcat(buf, castedByte);

    return data;
  }

  return 0;
}

void task_UART(void* pvParameters)
{
  params_UartTask_t params = *(params_UartTask_t*)pvParameters;

  void (*taskFunc)(char buf[], int data) = params.taskFunc;
  TAG_UART = params.tag;

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

  while (1) {
    taskFunc(buf, readUART(buf, UART_BUF_SIZE));
  }
}