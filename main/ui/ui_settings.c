#include "ui_settings.h"

lv_obj_t *page_settings = NULL;
lv_obj_t *page_wifi = NULL;
lv_obj_t *page_bluetooth = NULL;

static lv_obj_t *wifi_list = NULL;
static lv_obj_t *bt_list = NULL;
static lv_obj_t *wifi_status_label = NULL;

static void wifi_back_btn_cb(lv_event_t *e) { navigate_to(PAGE_SETTINGS); }
static void bt_back_btn_cb(lv_event_t *e) { navigate_to(PAGE_SETTINGS); }
static void nav_wifi_cb(lv_event_t *e) { navigate_to(PAGE_WIFI); }
static void nav_bluetooth_cb(lv_event_t *e) { navigate_to(PAGE_BLUETOOTH); }

static void wifi_toggle_cb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
    wifi_manager_start();
  } else {
    wifi_manager_stop();
  }
}

static void bt_toggle_cb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
    bluetooth_init();
    bluetooth_start_scan(10);
  } else {
    bluetooth_stop_scan();
    // bluetooth_disable();
  }
}

void create_settings_page(lv_obj_t *parent) {
  page_settings = lv_obj_create(parent);
  lv_obj_set_size(page_settings, LCD_H_RES, LCD_V_RES - 110);
  lv_obj_set_pos(page_settings, 0, 40);
  lv_obj_set_style_bg_color(page_settings, COLOR_BG_DARK, 0);
  lv_obj_clear_flag(page_settings, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(page_settings);
  lv_label_set_text(lbl, "Parametres");
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
  lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 10);

  // List of settings
  lv_obj_t *list = lv_obj_create(page_settings);
  lv_obj_set_size(list, LCD_H_RES - 40, LCD_V_RES - 100);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);

  // WiFi Button
  lv_obj_t *btn_wifi = lv_button_create(list);
  lv_obj_set_size(btn_wifi, lv_pct(100), 60);
  lv_obj_set_style_bg_color(btn_wifi, COLOR_BG_CARD, 0);
  lv_obj_add_event_cb(btn_wifi, nav_wifi_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *l_wifi = lv_label_create(btn_wifi);
  lv_label_set_text(l_wifi, LV_SYMBOL_WIFI " WiFi");
  lv_obj_center(l_wifi);

  // Bluetooth Button
  lv_obj_t *btn_bt = lv_button_create(list);
  lv_obj_set_size(btn_bt, lv_pct(100), 60);
  lv_obj_set_style_bg_color(btn_bt, COLOR_BG_CARD, 0);
  lv_obj_add_event_cb(btn_bt, nav_bluetooth_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *l_bt = lv_label_create(btn_bt);
  lv_label_set_text(l_bt, LV_SYMBOL_BLUETOOTH " Bluetooth");
  lv_obj_center(l_bt);
}

void create_wifi_page(lv_obj_t *parent) {
  page_wifi = lv_obj_create(parent);
  lv_obj_set_size(page_wifi, LCD_H_RES, LCD_V_RES - 110);
  lv_obj_set_pos(page_wifi, 0, 40);
  lv_obj_set_style_bg_color(page_wifi, COLOR_BG_DARK, 0);

  lv_obj_t *top = lv_obj_create(page_wifi);
  lv_obj_set_size(top, LCD_H_RES, 60);
  lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(top, 0, 0);

  lv_obj_t *btn_back = lv_button_create(top);
  lv_obj_set_size(btn_back, 100, 40);
  lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 10, 0);
  lv_obj_add_event_cb(btn_back, wifi_back_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_label_set_text(lv_label_create(btn_back), "Retour");

  lv_obj_t *sw = lv_switch_create(top);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_add_event_cb(sw, wifi_toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);
  if (wifi_manager_is_enabled())
    lv_obj_add_state(sw, LV_STATE_CHECKED);

  wifi_status_label = lv_label_create(page_wifi);
  lv_label_set_text(wifi_status_label, "Pret.");
  lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 70);

  wifi_list = lv_obj_create(page_wifi);
  lv_obj_set_size(wifi_list, LCD_H_RES - 20, LCD_V_RES - 200);
  lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 100);
  // ... WiFi list filling logic normally goes here or in a generic update
  // function
}

void create_bluetooth_page(lv_obj_t *parent) {
  page_bluetooth = lv_obj_create(parent);
  lv_obj_set_size(page_bluetooth, LCD_H_RES, LCD_V_RES - 110);
  lv_obj_set_pos(page_bluetooth, 0, 40);
  lv_obj_set_style_bg_color(page_bluetooth, COLOR_BG_DARK, 0);

  lv_obj_t *top = lv_obj_create(page_bluetooth);
  lv_obj_set_size(top, LCD_H_RES, 60);
  lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);

  lv_obj_t *btn_back = lv_button_create(top);
  lv_obj_set_size(btn_back, 100, 40);
  lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 10, 0);
  lv_obj_add_event_cb(btn_back, bt_back_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_label_set_text(lv_label_create(btn_back), "Retour");

  lv_obj_t *sw = lv_switch_create(top);
  lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_add_event_cb(sw, bt_toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);

  bt_list = lv_obj_create(page_bluetooth);
  lv_obj_set_size(bt_list, LCD_H_RES - 20, LCD_V_RES - 200);
  lv_obj_align(bt_list, LV_ALIGN_TOP_MID, 0, 100);
}
