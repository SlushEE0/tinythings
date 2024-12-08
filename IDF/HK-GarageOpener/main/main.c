#include "freertos/FreeRTOS.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "config.h"
#include <esp_https_ota.h>

static const char *TAG = "MAIN";

esp_netif_t *netif;

esp_err_t connectWifi() {
  esp_err_t nvsErrCheck = nvs_flash_init();
  if (nvsErrCheck == ESP_ERR_NVS_NO_FREE_PAGES ||
      nvsErrCheck == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvsErrCheck = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvsErrCheck);

  const wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t wifiSTAConfig = {
    .sta = {.ssid = {WIFI_SSID}, .password = {WIFI_PASS}, .channel = 0}};

  // starts esp event loop that wifi uses to connect, custom handle can be made
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_netif_init());

  esp_netif_config_t netifCfg = ESP_NETIF_DEFAULT_WIFI_STA();

  netif = esp_netif_new(&netifCfg);
  assert(netif);
  ESP_ERROR_CHECK(esp_netif_attach_wifi_station(netif));
  ESP_ERROR_CHECK(esp_wifi_set_default_wifi_sta_handlers());

  ESP_ERROR_CHECK(esp_wifi_init(&wifiConfig));

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_err_t ret;
  ret = esp_wifi_connect();
  ESP_LOGI(TAG, "connecting to wifi");

  while (ret != ESP_OK) {
    ESP_LOGW(TAG, "ret: %d", ret);
    ESP_LOGI(TAG, ".");
    ESP_ERROR_CHECK(ret);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  ESP_LOGI(TAG, "WIFI CONNECTED");
  esp_wifi_set_ps(WIFI_PS_NONE);

  return ESP_OK;
}

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

void ota_upgrade() {
  esp_http_client_config_t cfg_https = {
    .url = OTA_UPGRADE_URL,
    .keep_alive_enable = true,
    .cert_pem = (char *)server_cert_pem_start,
  };

  esp_https_ota_config_t cfg_ota = {
    .http_config = &cfg_https,
  };

  ESP_LOGI(
    TAG, "Attempting to download update from %s", cfg_ota.http_config->url);

  esp_err_t ret = esp_https_ota(&cfg_ota);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
    esp_restart();
  } else {
    ESP_LOGE(TAG, "Firmware upgrade failed");
  }
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

bool ota_verify() {
  //! VERIFY OTA

  return true;
}

esp_err_t ota_startup() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      // run diagnostic function ...
      bool diagnostic_is_ok = ota_verify();
      if (diagnostic_is_ok) {
        ESP_LOGI(
          TAG, "Diagnostics completed successfully! Continuing execution ...");
        esp_ota_mark_app_valid_cancel_rollback();
      } else {
        ESP_LOGE(
          TAG,
          "Diagnostics failed! Start rollback to the previous version ...");
        esp_ota_mark_app_invalid_rollback_and_reboot();
      }
    }
  }
}

void app_main(void) {
  gpio_set_direction(GPIO_NUM_11, GPIO_MODE_OUTPUT);

  while (1) {
    printf("Turning on the LED\n");
    gpio_set_level(GPIO_NUM_11, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Turning off the LED\n");
    gpio_set_level(GPIO_NUM_11, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}