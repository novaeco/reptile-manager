/**
 * @file ui_assets.h
 * @brief UI Assets (images, fonts, keyboard maps)
 */

#ifndef UI_ASSETS_H
#define UI_ASSETS_H

#include "lvgl.h"

// ====================================================================================
// KEYBOARD CONTROLS & FLAGS
// ====================================================================================

// Flag to mark mode-switching buttons
#define KB_CTRL_MODE_BTN                                                       \
  (LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_NO_REPEAT |             \
   LV_BUTTONMATRIX_CTRL_CLICK_TRIG)

// ====================================================================================
// KEYBOARD MAPS (AZERTY)
// ====================================================================================

extern const char *kb_map_azerty_lower[];
extern const char *kb_map_azerty_upper[];
extern const char *kb_map_special[];

// ====================================================================================
// KEYBOARD CONTROL MAPS
// ====================================================================================

extern const lv_buttonmatrix_ctrl_t kb_ctrl_lower[];
extern const lv_buttonmatrix_ctrl_t kb_ctrl_upper[];
extern const lv_buttonmatrix_ctrl_t kb_ctrl_special[];

#endif // UI_ASSETS_H
