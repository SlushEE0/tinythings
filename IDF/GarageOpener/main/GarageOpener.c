#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include "config.h"
#include "secrets.h"
#include "uss.h"

#define CONFIG_USE_HTTPS 0

#define API_SECRET ESPHOME_GARAGE_API_AUTH_KEY
#define USE_AUTH (0)

#define TAG ("ESPHOME.Garage")

t_GarageObj garageObj = {.readState = false, .isOpen = false};

esp_err_t handlePUT_garage(httpd_req_t *req);
esp_err_t handleOPTIONS_garage(httpd_req_t *req);
esp_err_t handlePOST_garage(httpd_req_t *req);
esp_err_t handleGET_garage(httpd_req_t *req);
esp_err_t connectWifi();
esp_err_t initServer();
esp_err_t initGpio(int pin);
void setGarageState(char *state, int force, int forceTime);
void sendRes(httpd_req_t *req, int err, char *res);
void initNetif();

void initNetif() {
  esp_netif_config_t netifCfg = ESP_NETIF_DEFAULT_WIFI_STA();

  esp_netif_t *netif = esp_netif_new(&netifCfg);
  assert(netif);
  ESP_ERROR_CHECK(esp_netif_attach_wifi_station(netif));
  ESP_ERROR_CHECK(esp_wifi_set_default_wifi_sta_handlers());
}

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
  initNetif();
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

  return ESP_OK;
}

esp_err_t initServer() {
  static httpd_handle_t serverHandle;

#if CONFIG_USE_HTTPS
  httpd_ssl_config_t serverCfg = HTTPD_SSL_CONFIG_DEFAULT();

  serverCfg.httpd.recv_wait_timeout = 7;

  extern const unsigned char servercert_start[] asm(
    "_binary_servercert_pem_start");
  extern const unsigned char servercert_end[] asm("_binary_servercert_pem_end");
  serverCfg.servercert = servercert_start;
  serverCfg.servercert_len = servercert_end - servercert_start;

  extern const unsigned char prvtkey_pem_start[] asm(
    "_binary_prvtkey_pem_start");
  extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");
  serverCfg.prvtkey_pem = prvtkey_pem_start;
  serverCfg.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

  if (httpd_ssl_start(&serverHandle, &serverCfg) != ESP_OK) {
    ESP_LOGE(TAG, "rest server init failed");
    return ESP_FAIL;
  }
#else
  httpd_config_t serverCfg = HTTPD_DEFAULT_CONFIG();

  if (httpd_start(&serverHandle, &serverCfg) != ESP_OK) {
    ESP_LOGE(TAG, "rest server init failed");
    return ESP_FAIL;
  }
#endif

  ESP_LOGI(TAG, "httpd_start worked");

  esp_wifi_set_ps(WIFI_PS_NONE);

  httpd_uri_t garagePOSTcfg = {.uri = "/api/garage",
                               .method = HTTP_POST,
                               .handler = handlePOST_garage,
                               .user_ctx = NULL};

  httpd_uri_t garageGETcfg = {.uri = "/api/garage",
                              .method = HTTP_GET,
                              .handler = handleGET_garage,
                              .user_ctx = NULL};

  httpd_uri_t garagePUTcfg = {.uri = "/api/garage",
                              .method = HTTP_PUT,
                              .handler = handlePUT_garage,
                              .user_ctx = NULL};

  httpd_uri_t garageOPTIONScfg = {.uri = "/api/garage",
                                  .method = HTTP_OPTIONS,
                                  .handler = handleOPTIONS_garage,
                                  .user_ctx = NULL};

  httpd_register_uri_handler(serverHandle, &garagePOSTcfg);
  httpd_register_uri_handler(serverHandle, &garageOPTIONScfg);
  httpd_register_uri_handler(serverHandle, &garagePUTcfg);
  httpd_register_uri_handler(serverHandle, &garageGETcfg);

  ESP_LOGI(TAG, "rest uris init ok");

  return ESP_OK;
};

void sendRes(httpd_req_t *req, int err, char *res) {
  httpd_resp_set_type(req, "text/json");

  ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_set_hdr(
    req, (const char *)"Access-Control-Allow-Origin", (const char *)"*"));
  ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_set_hdr(
    req, (const char *)"Access-Control-Allow-Headers", (const char *)"*"));

  if (err) httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, res);

  ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send(req, res, strlen(res)));
}

esp_err_t handleOPTIONS_garage(httpd_req_t *req) {
  ESP_LOGI(TAG, "OPTIONS req recieved");

  ESP_ERROR_CHECK_WITHOUT_ABORT(
    httpd_resp_set_hdr(req,
                       (const char *)"Access-Control-Allow-Methods",
                       (const char *)"OPTIONS, GET, POST, PUT"));

  sendRes(req, 1, HTTPD_204);

  return ESP_OK;
}

/// @brief
/// toggles garage state
esp_err_t handlePUT_garage(httpd_req_t *req) {
  ESP_LOGI(TAG, "PUT req recieved");
  ESP_LOGI(TAG, "request length: %d", req->content_len);

  char buf[256];
  httpd_req_recv(req, buf, req->content_len);

  ESP_LOGI(TAG, "request:  %s", buf);

  cJSON *reqBodyJSON = cJSON_Parse(buf);
  cJSON *forceTimeObj = cJSON_GetObjectItemCaseSensitive(reqBodyJSON, "force");

  int forceTime = 0;

  if (forceTimeObj != NULL) {
    forceTime = forceTimeObj->valueint;
  }

  setGarageState(NULL, 1, forceTime);
  sendRes(req, 0, "open/closing");

  return ESP_OK;
}
/// @brief
/// sets garage state to body.setState
/// open or close
/// @return
esp_err_t handlePOST_garage(httpd_req_t *req) {
  ESP_LOGI(TAG, "POST req recieved");

  char buf[256];

  httpd_req_recv(req, buf, req->content_len);

  cJSON *reqBodyJSON = cJSON_Parse(buf);
  cJSON *forceTimeObj = cJSON_GetObjectItemCaseSensitive(reqBodyJSON, "force");

  int forceTime = 0;

  if (forceTimeObj != NULL) {
    forceTime = forceTimeObj->valueint;
  }

  char *reqGarageState =
    cJSON_GetObjectItem(reqBodyJSON, "setState")->valuestring;

  for (int i = 0; reqGarageState[i]; i++)
    reqGarageState[i] = tolower(reqGarageState[i]);

  if ((strcmp(reqGarageState, "open") == 0) ||
      (strcmp(reqGarageState, "close") == 0)) {
    setGarageState(reqGarageState, 0, forceTime);

    sendRes(req, 0, reqGarageState);

    return ESP_OK;
  }

  httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "expected 'open' or 'close'");
  ESP_LOGW(TAG, "expected 'open' or 'close' but got %s", reqGarageState);
  return ESP_FAIL;
}

/// @brief
/// gets current garage state
/// @return
esp_err_t handleGET_garage(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET req recieved");

  char *res = "";
  garageObj.readState = true;
  vTaskDelay(300 / portTICK_PERIOD_MS);

  if (garageObj.isOpen) res = "open";
  if (!garageObj.isOpen) res = "close";
  garageObj.readState = false;
  sendRes(req, 0, res);
  return ESP_OK;
}

esp_err_t initGpio(int pin) {
  gpio_reset_pin(pin);
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
  gpio_set_level(pin, 0);

  ESP_LOGI(TAG, "gpio initialized");

  return ESP_OK;
}

void triggerRelay(int forceTime) {
  if (forceTime == 0 || forceTime < 30000) forceTime = 400;

  gpio_set_level(RELAY_PIN, 1);
  vTaskDelay(forceTime / portTICK_PERIOD_MS);
  gpio_set_level(RELAY_PIN, 0);

  ESP_LOGI(TAG, "Relay Triggered");
}

void setGarageState(char *state, int toggle, int forceTime) {
  ESP_LOGI(TAG, "REQ: %s", state);

  if (toggle) {
    triggerRelay(forceTime);

    if (garageObj.isOpen) {
      garageObj.isOpen = false;
    } else {
      garageObj.isOpen = true;
    }
  } else if (state != NULL) {
    if (garageObj.isOpen && (strcmp(state, "close") == 0))
      triggerRelay(forceTime);
    if (!garageObj.isOpen && (strcmp(state, "open") == 0))
      triggerRelay(forceTime);

    if (strcmp(state, "close") == 0) {
      garageObj.isOpen = false;
    } else {
      garageObj.isOpen = true;
    }
  }

  return;
}

void app_main(void) {
  ESP_ERROR_CHECK(connectWifi(WIFI_SSID, WIFI_PASS));
  ESP_ERROR_CHECK(initGpio(RELAY_PIN));
  ESP_ERROR_CHECK(initServer());
  xTaskCreate(&tsk_ussRead,
              "Reader Task",
              2048,
              (void *)&garageObj,
              portPRIVILEGE_BIT,
              NULL);

  return;
}
