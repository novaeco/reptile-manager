#include "wifi_manager.h"
#include "esp_event.h"
#include "esp_hosted.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "string.h"

// Define TAG
static const char *TAG = "WIFI_MANAGER";

// Internal state
static bool wifi_initialized = false;
static bool wifi_enabled = false;
static bool wifi_connected = false;
static char wifi_ssid[33] = "";
static char wifi_ip[16] = "0.0.0.0";
static char wifi_status_msg[64] = "";

// Selected credentials for connection
static char wifi_selected_ssid[33] = "";
static char wifi_password_input[65] = "";

// Scan results
static wifi_ap_record_t wifi_scan_results[WIFI_SCAN_MAX_AP];
static uint16_t wifi_scan_count = 0;

// NVS Keys
#define NVS_WIFI_NAMESPACE "wifi_creds"
#define NVS_WIFI_SSID_KEY "saved_ssid"
#define NVS_WIFI_PASS_KEY "saved_pass"

// Event handler forward declaration
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

// ====================================================================================
// PUBLIC API IMPLEMENTATION
// ====================================================================================

esp_err_t wifi_manager_init(void) {
  if (wifi_initialized) {
    return ESP_OK;
  }

  // esp_netif_init() is called in app_main

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t ret = esp_wifi_init(&cfg);
  if (ret != ESP_OK)
    return ret;

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));

  ret = esp_wifi_set_mode(WIFI_MODE_STA);
  if (ret == ESP_OK) {
    wifi_initialized = true;
  }
  return ret;
}

esp_err_t wifi_manager_start(void) {
  if (!wifi_initialized) {
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
      return ret;
    }
  }

  if (!wifi_enabled) {
    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK)
      return ret;
    wifi_enabled = true;
  }
  return ESP_OK;
}

esp_err_t wifi_manager_stop(void) {
  if (wifi_enabled) {
    esp_wifi_disconnect();
    esp_wifi_stop();
    wifi_enabled = false;
    wifi_connected = false;
  }
  return ESP_OK;
}

bool wifi_manager_is_enabled(void) { return wifi_enabled; }

bool wifi_manager_is_connected(void) { return wifi_connected; }

const char *wifi_manager_get_ssid(void) { return wifi_ssid; }

const char *wifi_manager_get_ip(void) { return wifi_ip; }

const char *wifi_manager_get_status_msg(void) { return wifi_status_msg; }

// Compare function for qsort
static int rssi_compare(const void *a, const void *b) {
  wifi_ap_record_t *ap_a = (wifi_ap_record_t *)a;
  wifi_ap_record_t *ap_b = (wifi_ap_record_t *)b;
  return ap_b->rssi - ap_a->rssi; // Descending order
}

esp_err_t wifi_manager_scan(void) {
  if (!wifi_enabled) {
    wifi_manager_start();
  }

  wifi_scan_config_t scan_config = {
      .show_hidden = true,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
  };

  // Note: Scanning is now non-blocking. Results will be available on
  // WIFI_EVENT_SCAN_DONE Check wifi_manager.h for event handling or use
  // wifi_manager_get_scan_results after a delay
  esp_wifi_scan_start(&scan_config, false); // Non-Blocking

  // Immediate result fetching removed as it returns 0 in async mode
  // Results are now handled in wifi_event_handler

  return ESP_OK;
}

void wifi_manager_get_scan_results(wifi_ap_record_t **results,
                                   uint16_t *count) {
  *results = wifi_scan_results;
  *count = wifi_scan_count;
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password) {
  if (!wifi_enabled)
    return ESP_ERR_INVALID_STATE;

  wifi_config_t wifi_cfg = {0};
  strlcpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid));
  strlcpy((char *)wifi_cfg.sta.password, password,
          sizeof(wifi_cfg.sta.password));
  wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  esp_wifi_disconnect();
  esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);

  // Update selected SSID/Pass for auto-save on success
  strncpy(wifi_selected_ssid, ssid, 32);
  strncpy(wifi_password_input, password, 64);

  return esp_wifi_connect();
}

esp_err_t wifi_manager_disconnect(void) { return esp_wifi_disconnect(); }

// ====================================================================================
// NVS STORAGE
// ====================================================================================

esp_err_t wifi_manager_save_credentials(const char *ssid,
                                        const char *password) {
  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
    return ret;
  }

  ret = nvs_set_str(nvs_handle, NVS_WIFI_SSID_KEY, ssid);
  if (ret != ESP_OK) {
    nvs_close(nvs_handle);
    return ret;
  }

  ret = nvs_set_str(nvs_handle, NVS_WIFI_PASS_KEY, password);
  nvs_close(nvs_handle);

  return ret;
}

esp_err_t wifi_manager_load_credentials(char *ssid, size_t ssid_len,
                                        char *password, size_t pass_len) {
  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(NVS_WIFI_NAMESPACE, NVS_READONLY, &nvs_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  ret = nvs_get_str(nvs_handle, NVS_WIFI_SSID_KEY, ssid, &ssid_len);
  if (ret != ESP_OK) {
    nvs_close(nvs_handle);
    return ret;
  }

  ret = nvs_get_str(nvs_handle, NVS_WIFI_PASS_KEY, password, &pass_len);
  nvs_close(nvs_handle);

  return ret;
}

bool wifi_manager_has_saved_credentials(void) {
  nvs_handle_t nvs_handle;
  if (nvs_open(NVS_WIFI_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
    return false;
  }

  size_t required_size = 0;
  esp_err_t ret =
      nvs_get_str(nvs_handle, NVS_WIFI_SSID_KEY, NULL, &required_size);
  nvs_close(nvs_handle);

  return (ret == ESP_OK && required_size > 1);
}

esp_err_t wifi_manager_delete_credentials(void) {
  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK)
    return ret;

  nvs_erase_key(nvs_handle, NVS_WIFI_SSID_KEY);
  nvs_erase_key(nvs_handle, NVS_WIFI_PASS_KEY);
  nvs_commit(nvs_handle);
  nvs_close(nvs_handle);
  return ESP_OK;
}

void wifi_manager_set_selected_ssid(const char *ssid) {
  strncpy(wifi_selected_ssid, ssid, 32);
}

const char *wifi_manager_get_selected_ssid(void) { return wifi_selected_ssid; }

void wifi_manager_set_password_input(const char *password) {
  strncpy(wifi_password_input, password, 64);
}

const char *wifi_manager_get_password_input(void) {
  return wifi_password_input;
}

// ====================================================================================
// EVENT HANDLER
// ====================================================================================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WiFi STA started");
      // Auto-connect logic is now handled in app_main or explicit call
      break;

    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to AP");
      {
        wifi_event_sta_connected_t *conn_event =
            (wifi_event_sta_connected_t *)event_data;
        snprintf(wifi_ssid, sizeof(wifi_ssid), "%s", conn_event->ssid);
      }
      // wifi_connected set on IP event
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGW(TAG, "Disconnected from AP");
      wifi_connected = false;
      wifi_ip[0] = 0;

      wifi_event_sta_disconnected_t *disc_event =
          (wifi_event_sta_disconnected_t *)event_data;

      snprintf(wifi_status_msg, sizeof(wifi_status_msg), "Auth Fail: %d",
               disc_event->reason);

      // Retry logic for simple cases
      if (wifi_enabled) {
        // Simple retry limit could be added here
        // esp_wifi_connect();
      }
      break;

    case WIFI_EVENT_SCAN_DONE:
      ESP_LOGI(TAG, "WiFi scan done");
      {
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        if (ap_count > WIFI_SCAN_MAX_AP)
          ap_count = WIFI_SCAN_MAX_AP;
        esp_wifi_scan_get_ap_records(&ap_count, wifi_scan_results);
        wifi_scan_count = ap_count;
        // Sort by RSSI
        qsort(wifi_scan_results, wifi_scan_count, sizeof(wifi_ap_record_t),
              rssi_compare);
      }
      break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    snprintf(wifi_ip, sizeof(wifi_ip), IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "Got IP: %s", wifi_ip);
    wifi_connected = true;
    snprintf(wifi_status_msg, sizeof(wifi_status_msg), "Connected: %s",
             wifi_ip);

    // Save successful credentials
    if (strlen(wifi_selected_ssid) > 0 && strlen(wifi_password_input) > 0) {
      wifi_manager_save_credentials(wifi_selected_ssid, wifi_password_input);
    }
  }
}
