#include <inttypes.h>
#include <stdbool.h>
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
#include "uss.h"

static char *TAG = "CONTROL";

bool garage_isOpen = false;
bool prevSend = false;

void triggerRelay(int time) {
  ESP_LOGI(TAG, "TRIGGERING RELAY");
  gpio_set_level(CONFIG_RELAY_PIN, 1);
  vTaskDelay(time / portTICK_PERIOD_MS);
  gpio_set_level(CONFIG_RELAY_PIN, 0);
};

bool setGarageState(bool isOpen) {
  if (garage_isOpen != isOpen) {
    triggerRelay(CONFIG_RELAY_TIME);
  }

  garage_isOpen = isOpen;

  return garage_isOpen;
}

void connectWifi() {
  esp_err_t nvsErrCheck = nvs_flash_init();
  if (nvsErrCheck == ESP_ERR_NVS_NO_FREE_PAGES ||
      nvsErrCheck == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvsErrCheck = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvsErrCheck);

  const wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t wifiSTAConfig = {.sta = {.ssid = {CONFIG_WIFI_SSID},
                                         .password = {CONFIG_WIFI_PASS},
                                         .channel = 0}};

  // starts esp event loop that wifi uses to connect, custom handle can be made
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_netif_init());

  esp_netif_config_t netifCfg = ESP_NETIF_DEFAULT_WIFI_STA();

  esp_netif_t *netif = esp_netif_new(&netifCfg);
  assert(netif);
  ESP_ERROR_CHECK(esp_netif_attach_wifi_station(netif));
  ESP_ERROR_CHECK(esp_wifi_set_default_wifi_sta_handlers());

  ESP_ERROR_CHECK(esp_wifi_init(&wifiConfig));

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_ERROR_CHECK(esp_wifi_connect());

  ESP_LOGI(TAG, "WIFI CONNECTING");

#ifndef CONFIG_WIFI_PS
  esp_wifi_set_ps(WIFI_PS_NONE);
#endif
}

void sendRes(httpd_req_t *req, char *res) {
  httpd_resp_set_type(req, "text/json");

  ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_set_hdr(
    req, (const char *)"Access-Control-Allow-Origin", (const char *)"*"));
  ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_set_hdr(
    req, (const char *)"Access-Control-Allow-Headers", (const char *)"*"));

  ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send(req, res, strlen(res)));
}

esp_err_t handler_POST(httpd_req_t *req) {
  garage_isOpen = garage_getIsOpen();

  char buf[256];
  httpd_req_recv(req, buf, req->content_len);
  cJSON *reqBody = cJSON_Parse(buf);

  cJSON *req_isOpen = cJSON_GetObjectItemCaseSensitive(reqBody, "isOpen");

  if (cJSON_IsBool(req_isOpen)) {
    setGarageState(req_isOpen->valueint);
  } else {
    ESP_LOGE(TAG, "POST: Expected property 'isOpen'");
    httpd_resp_send_err(
      req, HTTPD_400_BAD_REQUEST, "POST: Expected property 'isOpen'");
  }

  // send a response to the client
  sendRes(req, garage_isOpen ? "open" : "closed");
  return ESP_OK;
}

esp_err_t handler_GET(httpd_req_t *req) {
  garage_isOpen = garage_getIsOpen();

  cJSON *garageState = cJSON_CreateObject();
  cJSON_AddBoolToObject(garageState, "isOpen", garage_isOpen);

  sendRes(req, cJSON_Print(garageState));

  cJSON_Delete(garageState);

  return ESP_OK;
}

esp_err_t handler_PUT(httpd_req_t *req) {
  sendRes(req, setGarageState(!garage_isOpen) ? "open" : "closed");
  return ESP_OK;
}

esp_err_t handler_OPTIONS(httpd_req_t *req) {
  ESP_ERROR_CHECK_WITHOUT_ABORT(
    httpd_resp_set_hdr(req,
                       (const char *)"Access-Control-Allow-Methods",
                       (const char *)"OPTIONS, GET, POST, PUT"));

  sendRes(req, HTTPD_204);
  return ESP_OK;
}

struct async_resp_arg {
  httpd_handle_t hd;
  int fd;
};

static void ws_async_send(void *arg) {
  static const char *data = "Async data";
  struct async_resp_arg *resp_arg = arg;
  httpd_handle_t hd = resp_arg->hd;
  int fd = resp_arg->fd;
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.payload = (uint8_t *)data;
  ws_pkt.len = strlen(data);
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  httpd_ws_send_frame_async(hd, fd, &ws_pkt);
  free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req) {
  struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
  if (resp_arg == NULL) {
    return ESP_ERR_NO_MEM;
  }
  resp_arg->hd = req->handle;
  resp_arg->fd = httpd_req_to_sockfd(req);
  esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
  if (ret != ESP_OK) {
    free(resp_arg);
  }
  return ret;
}

static esp_err_t handler_WS(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "WS HANDSHAKE SUCCESS");
    return ESP_OK;
  }

  httpd_ws_frame_t ws_packet;
  uint8_t *buf = NULL;
  memset(&ws_packet, 0, sizeof(httpd_ws_frame_t));
  ws_packet.type = HTTPD_WS_TYPE_TEXT;
  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_packet, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "ERR:ln160 - httpd_ws_recv_frame failed with %d", ret);
    return ret;
  }

  ESP_LOGI(TAG, "PACKET LEN: %d", ws_packet.len);
  if (ws_packet.len) {
    // add one for null terminator
    buf = calloc(1, ws_packet.len + 1);
    if (buf == NULL) {
      ESP_LOGE(TAG, "ERR:ln170 - calloc failed");
      return ESP_ERR_NO_MEM;
    }
    ws_packet.payload = buf;

    ret = httpd_ws_recv_frame(req, &ws_packet, ws_packet.len);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      free(buf);
      return ret;
    }
    ESP_LOGI(TAG, "MSG: %s", ws_packet.payload);
  }

  if (prevSend != garage_isOpen) {
    const char *pkt_payload = garage_isOpen ? "open" : "closed";
    ESP_LOGI(TAG, "SENDING: %c", *pkt_payload);
    httpd_ws_frame_t ws_changePkt = {.type = HTTPD_WS_TYPE_TEXT,
                                     .payload = (uint8_t *)pkt_payload,
                                     .len = strlen(pkt_payload)};

    ret = httpd_ws_send_frame_async(
      req->handle, httpd_req_to_sockfd(req), &ws_changePkt);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }

    prevSend = garage_isOpen;
    return ret;
  }

  return ESP_OK;
  // if (req->method == HTTP_GET) {
  //   ESP_LOGI(TAG, "Handshake done, the new connection was opened");
  //   return ESP_OK;
  // }
  // httpd_ws_frame_t ws_pkt;
  // uint8_t *buf = NULL;
  // memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  // ws_pkt.type = HTTPD_WS_TYPE_TEXT;
  // /* Set max_len = 0 to get the frame len */
  // esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  // if (ret != ESP_OK) {
  //   ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d",
  //   ret); return ret;
  // }
  // ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
  // if (ws_pkt.len) {
  //   /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
  //   buf = calloc(1, ws_pkt.len + 1);
  //   if (buf == NULL) {
  //     ESP_LOGE(TAG, "Failed to calloc memory for buf");
  //     return ESP_ERR_NO_MEM;
  //   }
  //   ws_pkt.payload = buf;
  //   /* Set max_len = ws_pkt.len to get the frame payload */
  //   ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
  //   if (ret != ESP_OK) {
  //     ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
  //     free(buf);
  //     return ret;
  //   }
  //   ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
  // }
  // ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
  // if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
  //     strcmp((char *)ws_pkt.payload, "Trigger async") == 0) {
  //   free(buf);
  //   return trigger_async_send(req->handle, req);
  // }

  // ret = httpd_ws_send_frame(req, &ws_pkt);
  // if (ret != ESP_OK) {
  //   ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
  // }
  // free(buf);
  // return ret;
}

int ws_startServer() {
  httpd_handle_t server = NULL;
  httpd_config_t cfg_server = HTTPD_DEFAULT_CONFIG();

  ESP_LOGI(TAG, "STARTING WS SERVER, PORT: %d", cfg_server.server_port);

  esp_err_t ret = httpd_start(&server, &cfg_server);
  ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "REGISTERING URI HANDLERS");

    httpd_uri_t cfgG_POST = {
      .uri = "/control",
      .method = HTTP_POST,
      .handler = handler_POST,
      .user_ctx = NULL,
    };

    httpd_uri_t cfgG_GET = {.uri = "/control",
                            .method = HTTP_GET,
                            .handler = handler_GET,
                            .user_ctx = NULL};

    httpd_uri_t cfgG_PUT = {
      .uri = "/control",
      .method = HTTP_PUT,
      .handler = handler_PUT,
      .user_ctx = NULL,
    };

    httpd_uri_t cfgG_OPTIONS = {.uri = "/control",
                                .method = HTTP_OPTIONS,
                                .handler = handler_OPTIONS,
                                .user_ctx = NULL};

    httpd_uri_t cfgG_WS = {.uri = "/ws",
                           .method = HTTP_GET,
                           .handler = handler_WS,
                           .user_ctx = NULL,
                           .is_websocket = true};

    httpd_register_uri_handler(server, &cfgG_GET);
    httpd_register_uri_handler(server, &cfgG_POST);
    httpd_register_uri_handler(server, &cfgG_PUT);
    httpd_register_uri_handler(server, &cfgG_OPTIONS);
    httpd_register_uri_handler(server, &cfgG_WS);

    return 1;
  }

  ESP_LOGE(TAG, "ERROR STARTING WS SERVER");
  return 0;
}

void app_main() {
  ESP_ERROR_CHECK(gpio_reset_pin(CONFIG_RELAY_PIN));
  ESP_ERROR_CHECK(gpio_set_direction(CONFIG_RELAY_PIN, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_pull_mode(CONFIG_RELAY_PIN, GPIO_PULLDOWN_ENABLE));
  gpio_set_level(CONFIG_RELAY_PIN, 0);

  init_GPIO();
  connectWifi();

  ws_startServer();
}