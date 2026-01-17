/**
 * @file bluetooth_manager.h
 * @brief Bluetooth management for ESP32-P4 (via ESP-Hosted)
 */

#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include "esp_bt_defs.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

// ====================================================================================
// CONFIGURATION
// ====================================================================================

#define BT_SCAN_MAX_DEVICES 10
#define BLE_DEVICE_NAME_MAX_LEN 32

// ====================================================================================
// TYPES
// ====================================================================================

typedef struct {
  esp_bd_addr_t bda;
  char name[BLE_DEVICE_NAME_MAX_LEN + 1];
  int rssi;
  bool valid;
} bt_device_info_t;

// ====================================================================================
// PUBLIC API
// ====================================================================================

/**
 * @brief Initialize Bluetooth via ESP-Hosted
 * @return ESP_OK on success
 */
esp_err_t bluetooth_init(void);

/**
 * @brief Start BLE scan
 * @param duration_sec Scan duration in seconds
 * @return ESP_OK on success
 */
esp_err_t bluetooth_start_scan(uint32_t duration_sec);

/**
 * @brief Stop BLE scan
 * @return ESP_OK on success
 */
esp_err_t bluetooth_stop_scan(void);

// ====================================================================================
// GLOBAL STATE (To be accessed by UI)
// ====================================================================================

extern bt_device_info_t bt_scan_results[BT_SCAN_MAX_DEVICES];
extern int bt_scan_count;
extern bool bt_scanning;
extern bool bt_scan_update_pending;

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_MANAGER_H
