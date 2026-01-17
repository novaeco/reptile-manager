#include "bluetooth_manager.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#if CONFIG_BT_ENABLED
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_hosted.h"
#include "esp_hosted_bluedroid.h"

#endif

static const char *BT_TAG = "BLUETOOTH";

// ====================================================================================
// GLOBAL STATE
// ====================================================================================

bt_device_info_t bt_scan_results[BT_SCAN_MAX_DEVICES];
int bt_scan_count = 0;
bool bt_scanning = false;
bool bt_scan_update_pending = false;

static bool bt_initialized = false;

#if CONFIG_BT_ENABLED

// ====================================================================================
// HELPER FUNCTIONS
// ====================================================================================

// Helper function to convert BDA to string
static char *bda_to_str(esp_bd_addr_t bda, char *str, size_t size) {
  if (bda == NULL || str == NULL || size < 18) {
    return NULL;
  }
  sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", bda[0], bda[1], bda[2], bda[3],
          bda[4], bda[5]);
  return str;
}

// ====================================================================================
// CALLBACKS
// ====================================================================================

// BLE GAP event callback
static void bt_gap_ble_cb(esp_gap_ble_cb_event_t event,
                          esp_ble_gap_cb_param_t *param) {
  char bda_str[18];

  switch (event) {
  case ESP_GAP_BLE_SCAN_RESULT_EVT:
    if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
      // Found a BLE device
      ESP_LOGD(BT_TAG, "BLE Device found: %s, RSSI: %d",
               bda_to_str(param->scan_rst.bda, bda_str, sizeof(bda_str)),
               param->scan_rst.rssi);

      // Store device info if we have space - check for duplicates first
      int existing_idx = -1;
      for (int i = 0; i < bt_scan_count; i++) {
        if (memcmp(bt_scan_results[i].bda, param->scan_rst.bda,
                   sizeof(esp_bd_addr_t)) == 0) {
          existing_idx = i;
          break;
        }
      }

      // Use existing slot or new slot if not a duplicate
      int slot_idx = (existing_idx >= 0) ? existing_idx : bt_scan_count;

      if (slot_idx < BT_SCAN_MAX_DEVICES) {
        memcpy(bt_scan_results[slot_idx].bda, param->scan_rst.bda,
               sizeof(esp_bd_addr_t));
        bt_scan_results[slot_idx].rssi = param->scan_rst.rssi;

        // Try to get device name from advertising data
        uint8_t *adv_name = NULL;
        uint8_t adv_name_len = 0;
        adv_name = esp_ble_resolve_adv_data(
            param->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
        if (adv_name == NULL) {
          adv_name = esp_ble_resolve_adv_data(param->scan_rst.ble_adv,
                                              ESP_BLE_AD_TYPE_NAME_SHORT,
                                              &adv_name_len);
        }

        if (adv_name && adv_name_len > 0) {
          int copy_len = (adv_name_len > BLE_DEVICE_NAME_MAX_LEN)
                             ? BLE_DEVICE_NAME_MAX_LEN
                             : adv_name_len;
          memcpy(bt_scan_results[slot_idx].name, adv_name, copy_len);
          bt_scan_results[slot_idx].name[copy_len] = '\0';
          if (existing_idx < 0) {
            ESP_LOGI(BT_TAG, "  Name: %s", bt_scan_results[slot_idx].name);
          }
        } else if (existing_idx < 0 ||
                   strcmp(bt_scan_results[slot_idx].name, "(Unknown)") == 0) {
          // Only set to Unknown if it's new or was already Unknown
          snprintf(bt_scan_results[slot_idx].name,
                   sizeof(bt_scan_results[slot_idx].name), "(Unknown)");
        }

        bt_scan_results[slot_idx].valid = true;

        // Only increment count if this is a new device
        if (existing_idx < 0) {
          bt_scan_count++;
        }
      }
    } else if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
      ESP_LOGI(BT_TAG, "BLE Scan complete, found %d devices", bt_scan_count);
      bt_scanning = false;
      bt_scan_update_pending = true; // Signal UI update needed
    }
    break;

  case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(BT_TAG, "BLE scan started successfully");
      bt_scanning = true;
    } else {
      ESP_LOGE(BT_TAG, "BLE scan start failed: %d",
               param->scan_start_cmpl.status);
      bt_scanning = false;
    }
    break;

  case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    ESP_LOGI(BT_TAG, "BLE scan stopped");
    bt_scanning = false;
    break;

  case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    ESP_LOGI(BT_TAG, "Advertising data set complete");
    break;

  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(BT_TAG,
               "Advertising started - Device visible as 'Reptile Panel'");
    } else {
      ESP_LOGW(BT_TAG, "Advertising start failed: %d",
               param->adv_start_cmpl.status);
    }
    break;

  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    ESP_LOGI(BT_TAG, "Advertising stopped");
    break;

  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    ESP_LOGD(
        BT_TAG,
        "Connection params updated: status=%d, conn_int=%d, latency=%d, "
        "timeout=%d",
        param->update_conn_params.status, param->update_conn_params.conn_int,
        param->update_conn_params.latency, param->update_conn_params.timeout);
    break;

  default:
    ESP_LOGD(BT_TAG, "BLE GAP event: %d", event);
    break;
  }
}

// ====================================================================================
// PUBLIC API IMPLEMENTATION
// ====================================================================================

esp_err_t bluetooth_init(void) {
  if (bt_initialized) {
    return ESP_OK;
  }

  ESP_LOGI(BT_TAG, "Initializing Bluetooth via ESP32-C6...");

  // Ensure ESP-Hosted connection is established
  esp_err_t ret = esp_hosted_connect_to_slave();
  if (ret != ESP_OK) {
    ESP_LOGE(BT_TAG,
             "Failed to connect to ESP-Hosted slave: %s. Bluetooth will be "
             "disabled.",
             esp_err_to_name(ret));
    return ret;
  }

  // Initialize Bluetooth controller on ESP32-C6
  ret = esp_hosted_bt_controller_init();
  if (ret != ESP_OK) {
    ESP_LOGW(BT_TAG, "BT controller init: %s (may already be initialized)",
             esp_err_to_name(ret));
    // Continue anyway, might already be initialized
  }

  // Enable Bluetooth controller
  ret = esp_hosted_bt_controller_enable();
  if (ret != ESP_OK) {
    ESP_LOGW(BT_TAG, "BT controller enable: %s (may already be enabled)",
             esp_err_to_name(ret));
    // Continue anyway, might already be enabled
  }

  // Open HCI transport for Bluedroid
  hosted_hci_bluedroid_open();

  // Get and attach HCI driver operations
  esp_bluedroid_hci_driver_operations_t hci_ops = {
      .send = hosted_hci_bluedroid_send,
      .check_send_available = hosted_hci_bluedroid_check_send_available,
      .register_host_callback = hosted_hci_bluedroid_register_host_callback,
  };
  esp_bluedroid_attach_hci_driver(&hci_ops);

  // Initialize Bluedroid stack
  ret = esp_bluedroid_init();
  if (ret != ESP_OK) {
    ESP_LOGE(BT_TAG, "Failed to init Bluedroid: %s", esp_err_to_name(ret));
    return ret;
  }

  // Enable Bluedroid
  ret = esp_bluedroid_enable();
  if (ret != ESP_OK) {
    ESP_LOGE(BT_TAG, "Failed to enable Bluedroid: %s", esp_err_to_name(ret));
    return ret;
  }

  // Register BLE GAP callback
  ret = esp_ble_gap_register_callback(bt_gap_ble_cb);
  if (ret != ESP_OK) {
    ESP_LOGE(BT_TAG, "Failed to register BLE GAP callback: %s",
             esp_err_to_name(ret));
    return ret;
  }

  // Set device name - "Reptile Panel"
  esp_ble_gap_set_device_name("Reptile Panel");

  // Configure BLE advertising parameters
  esp_ble_adv_params_t adv_params = {
      .adv_int_min = 0x20,      // 20ms minimum interval
      .adv_int_max = 0x40,      // 40ms maximum interval
      .adv_type = ADV_TYPE_IND, // Connectable undirected advertising
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };

  // Configure advertising data
  esp_ble_adv_data_t adv_data = {
      .set_scan_rsp = false,
      .include_name = true,
      .include_txpower = true,
      .min_interval = 0x0006, // 7.5ms
      .max_interval = 0x0010, // 20ms
      .appearance = 0x00,
      .manufacturer_len = 0,
      .p_manufacturer_data = NULL,
      .service_data_len = 0,
      .p_service_data = NULL,
      .service_uuid_len = 0,
      .p_service_uuid = NULL,
      .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
  };

  // Set advertising data
  ret = esp_ble_gap_config_adv_data(&adv_data);
  if (ret != ESP_OK) {
    ESP_LOGW(BT_TAG, "Failed to config adv data: %s", esp_err_to_name(ret));
  }

  // Start advertising (will be visible to other devices)
  ret = esp_ble_gap_start_advertising(&adv_params);
  if (ret != ESP_OK) {
    ESP_LOGW(BT_TAG, "Failed to start advertising: %s", esp_err_to_name(ret));
  } else {
    ESP_LOGI(BT_TAG, "BLE Advertising started - Device name: 'Reptile Panel'");
  }

  bt_initialized = true;
  ESP_LOGI(BT_TAG, "Bluetooth initialized successfully");
  return ESP_OK;
}

esp_err_t bluetooth_start_scan(uint32_t duration_sec) {
  if (!bt_initialized) {
    ESP_LOGW(BT_TAG, "Bluetooth not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  // Always try to stop first if scanning
  if (bt_scanning) {
    ESP_LOGI(BT_TAG, "Stopping ongoing scan before restart...");
    esp_ble_gap_stop_scanning();
    bt_scanning = false;
    vTaskDelay(pdMS_TO_TICKS(200)); // Wait for scan to fully stop
  }

  // Clear previous scan results
  memset(bt_scan_results, 0, sizeof(bt_scan_results));
  bt_scan_count = 0;

  // Configure scan parameters
  esp_ble_scan_params_t scan_params = {
      .scan_type = BLE_SCAN_TYPE_ACTIVE,
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
      .scan_interval = 0x50, // 50ms
      .scan_window = 0x30,   // 30ms
      .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
  };

  esp_err_t ret = esp_ble_gap_set_scan_params(&scan_params);
  if (ret != ESP_OK) {
    ESP_LOGE(BT_TAG, "Failed to set scan params: %s", esp_err_to_name(ret));
    return ret;
  }

  // Start scanning
  ret = esp_ble_gap_start_scanning(duration_sec);
  if (ret != ESP_OK) {
    ESP_LOGE(BT_TAG, "Failed to start scan: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(BT_TAG, "BLE scan started for %lu seconds", duration_sec);
  return ESP_OK;
}

esp_err_t bluetooth_stop_scan(void) {
  if (!bt_initialized || !bt_scanning) {
    return ESP_OK;
  }

  return esp_ble_gap_stop_scanning();
}

#else // CONFIG_BT_ENABLED not defined

esp_err_t bluetooth_init(void) {
  ESP_LOGW(BT_TAG, "Bluetooth disabled in sdkconfig - skipping init");
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bluetooth_start_scan(uint32_t duration_sec) {
  (void)duration_sec;
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bluetooth_stop_scan(void) { return ESP_ERR_NOT_SUPPORTED; }

#endif // CONFIG_BT_ENABLED
