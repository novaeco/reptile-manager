/**
 * @file ui_manager.h
 * @brief User Interface Manager for Reptile Manager
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h"

/**
 * @brief Initialize the User Interface
 * @param display_handle Pointer to the LVGL display handle
 */
void ui_init(lv_display_t *display_handle);

/**
 * @brief Update the Status Bar (Time, WiFi, Battery)
 * Should be called periodically from the main loop
 */
void ui_update_status_bar(void);

// Hardware accessors (implemented in main.c or hardware module, exposed here
// for UI to use) Alternatively, UI callbacks can call these if they are extern.
// For now, we will assume main.c exposes:
// - backlight_set(uint8_t)
// - wifi_manager / bluetooth_manager are self-contained

#endif // UI_MANAGER_H
