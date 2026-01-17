#ifndef UI_SHARED_H
#define UI_SHARED_H

#include "bluetooth_manager.h"
#include "data/database.h"
#include "lvgl.h"
#include "models.h"
#include "ui_assets.h"
#include "ui_theme.h"
#include "wifi_manager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Global UI Constants
#define LCD_H_RES 480
#define LCD_V_RES 800
#define LC_ITEMS_HEIGHT 70

// Page IDs
typedef enum {
  PAGE_HOME = 0,
  PAGE_ANIMALS,
  PAGE_ANIMAL_DETAIL,
  PAGE_BREEDING,
  PAGE_GALLERY,
  PAGE_CONFORMITY,
  PAGE_SETTINGS,
  PAGE_WIFI,
  PAGE_BLUETOOTH,
  PAGE_DIAGNOSTICS
} page_id_t;

// Shared State
extern int selected_animal_id;
extern page_id_t current_page;
extern lv_obj_t *ui_status_bar;
extern lv_obj_t *ui_navbar;

// Shared Helper Functions (implemented in ui_manager.c usually, or a new
// ui_helpers.c)
void navigate_to(page_id_t page);
void format_date(time_t timestamp, char *buf, size_t len);
lv_obj_t *create_card(lv_obj_t *parent, int w, int h);
lv_obj_t *create_button(lv_obj_t *parent, const char *text, int w, int h);
void show_toast(const char *msg, lv_color_t color);

// Shared Callbacks (implemented in respective pages or ui_manager)
void close_popup_cb(lv_event_t *e);

// Species/Icon Helpers
// Species/Icon Helpers
const char *reptile_get_icon(reptile_species_t species);
#endif // UI_SHARED_H
