#include "ui_home.h"

// Local handles
static lv_obj_t *label_time = NULL;
static lv_obj_t *label_date = NULL;
static lv_obj_t *icon_wifi = NULL;
static lv_obj_t *icon_bluetooth = NULL;
lv_obj_t *page_home = NULL;

// Navigation Callbacks
static void nav_home_cb(lv_event_t *e) { navigate_to(PAGE_HOME); }
static void nav_animals_cb(lv_event_t *e) { navigate_to(PAGE_ANIMALS); }
static void nav_breeding_cb(lv_event_t *e) { navigate_to(PAGE_BREEDING); }
static void nav_gallery_cb(lv_event_t *e) { navigate_to(PAGE_GALLERY); }
static void nav_conformity_cb(lv_event_t *e) { navigate_to(PAGE_CONFORMITY); }
static void nav_settings_cb(lv_event_t *e) { navigate_to(PAGE_SETTINGS); }

void create_status_bar(lv_obj_t *parent) {
  ui_status_bar = lv_obj_create(parent);
  lv_obj_set_size(ui_status_bar, LCD_H_RES, 40);
  lv_obj_align(ui_status_bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(ui_status_bar, COLOR_HEADER, 0);
  lv_obj_set_style_bg_opa(ui_status_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ui_status_bar, 0, 0);
  lv_obj_set_style_pad_hor(ui_status_bar, 10, 0);
  lv_obj_clear_flag(ui_status_bar, LV_OBJ_FLAG_SCROLLABLE);

  // Time & Date
  label_time = lv_label_create(ui_status_bar);
  lv_label_set_text(label_time, "00:00");
  lv_obj_set_style_text_color(label_time, COLOR_TEXT, 0);
  lv_obj_align(label_time, LV_ALIGN_RIGHT_MID, 0, 0);

  label_date = lv_label_create(ui_status_bar);
  lv_label_set_text(label_date, "...");
  lv_obj_set_style_text_color(label_date, COLOR_TEXT, 0);
  lv_obj_align(label_date, LV_ALIGN_RIGHT_MID, -60, 0);

  // Status Icons
  icon_wifi = lv_label_create(ui_status_bar);
  lv_label_set_text(icon_wifi, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(icon_wifi, COLOR_TEXT_DIM, 0);
  lv_obj_align(icon_wifi, LV_ALIGN_LEFT_MID, 0, 0);

  icon_bluetooth = lv_label_create(ui_status_bar);
  lv_label_set_text(icon_bluetooth, LV_SYMBOL_BLUETOOTH);
  lv_obj_set_style_text_color(icon_bluetooth, COLOR_TEXT_DIM, 0);
  lv_obj_align(icon_bluetooth, LV_ALIGN_LEFT_MID, 25, 0);

  // Settings Button
  lv_obj_t *sets_btn = lv_button_create(ui_status_bar);
  lv_obj_set_size(sets_btn, 30, 30);
  lv_obj_align(sets_btn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_opa(sets_btn, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(sets_btn, nav_settings_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sets_lbl = lv_label_create(sets_btn);
  lv_label_set_text(sets_lbl, LV_SYMBOL_SETTINGS);
  lv_obj_center(sets_lbl);
}

void create_navbar(lv_obj_t *parent) {
  ui_navbar = lv_obj_create(parent);
  lv_obj_set_size(ui_navbar, LCD_H_RES, 70);
  lv_obj_align(ui_navbar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(ui_navbar, COLOR_HEADER, 0);
  lv_obj_set_style_bg_grad_color(ui_navbar, COLOR_HEADER_GRADIENT, 0);
  lv_obj_set_style_bg_grad_dir(ui_navbar, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_width(ui_navbar, 0, 0);
  lv_obj_set_style_radius(ui_navbar, 20, 0);
  lv_obj_set_style_pad_all(ui_navbar, 5, 0);
  lv_obj_clear_flag(ui_navbar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(ui_navbar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ui_navbar, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  const char *icons[] = {LV_SYMBOL_HOME, LV_SYMBOL_LIST, LV_SYMBOL_SHUFFLE,
                         LV_SYMBOL_IMAGE, LV_SYMBOL_FILE};
  const char *titles[] = {"Accueil", "Animaux", "Repro", "Galerie", "Registre"};
  lv_event_cb_t callbacks[] = {nav_home_cb, nav_animals_cb, nav_breeding_cb,
                               nav_gallery_cb, nav_conformity_cb};

  for (int i = 0; i < 5; i++) {
    lv_obj_t *btn = lv_button_create(ui_navbar);
    lv_obj_set_size(btn, 70, 60);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(btn, callbacks[i], LV_EVENT_CLICKED, NULL);

    lv_obj_t *ico = lv_label_create(btn);
    lv_label_set_text(ico, icons[i]);
    lv_obj_set_style_text_font(ico, &lv_font_montserrat_24, 0);

    lv_obj_t *txt = lv_label_create(btn);
    lv_label_set_text(txt, titles[i]);
    lv_obj_set_style_text_font(txt, &lv_font_montserrat_10, 0);
  }
}

void update_status_bar(void) {
  if (!label_time || !label_date)
    return;

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char buf[16];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  lv_label_set_text(label_time, buf);

  strftime(buf, sizeof(buf), "%d/%m", &timeinfo);
  lv_label_set_text(label_date, buf);

  // Update WiFi Icon
  if (wifi_manager_is_connected()) {
    lv_obj_set_style_text_color(icon_wifi, COLOR_SUCCESS, 0);
  } else if (wifi_manager_is_enabled()) {
    lv_obj_set_style_text_color(icon_wifi, lv_color_hex(0xFF9800), 0); // Orange
  } else {
    lv_obj_set_style_text_color(icon_wifi, COLOR_TEXT_DIM, 0);
  }

  // Update BT Icon - Assuming basic check
  // extern bool bluetooth_enabled; // From somewhere?
  // For now purely visual based on assumptions or getter
  // We can add bluetooth_manager_is_enabled() later
}

void create_home_page(lv_obj_t *parent) {
  page_home = lv_obj_create(parent);
  lv_obj_set_size(page_home, LCD_H_RES, LCD_V_RES - 40 - 70);
  lv_obj_set_pos(page_home, 0, 40);
  lv_obj_set_style_bg_color(page_home, COLOR_BG_DARK, 0);
  lv_obj_set_style_border_width(page_home, 0, 0);
  lv_obj_clear_flag(page_home, LV_OBJ_FLAG_SCROLLABLE);

  // Logo / Header Image
  lv_obj_t *logo_lbl = lv_label_create(page_home);
  lv_label_set_text(logo_lbl, "REPTILE MANAGER");
  lv_obj_set_style_text_font(logo_lbl, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(logo_lbl, COLOR_PRIMARY, 0);
  lv_obj_align(logo_lbl, LV_ALIGN_TOP_MID, 0, 15);

  // Dashboard Grid
  int gap = 15;
  int margin = 20;
  int card_w = (LCD_H_RES - (margin * 2) - gap) / 2;
  int row_y = 60;

  // --- Card: Animals ---
  lv_obj_t *card_anim = create_card(page_home, card_w, 120);
  lv_obj_set_pos(card_anim, margin, row_y);
  lv_obj_add_flag(card_anim, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card_anim, nav_animals_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *icon_anim = lv_label_create(card_anim);
  lv_label_set_text(icon_anim, LV_SYMBOL_LIST);
  lv_obj_set_style_text_font(icon_anim, &lv_font_montserrat_34, 0);
  lv_obj_set_style_text_color(icon_anim, COLOR_SNAKE, 0);
  lv_obj_align(icon_anim, LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t *lbl_anim_count = lv_label_create(card_anim);
  lv_label_set_text_fmt(lbl_anim_count, "%d", reptile_count);
  lv_obj_set_style_text_font(lbl_anim_count, &lv_font_montserrat_34, 0);
  lv_obj_align(lbl_anim_count, LV_ALIGN_BOTTOM_LEFT, 0, -20);

  lv_obj_t *lbl_anim_txt = lv_label_create(card_anim);
  lv_label_set_text(lbl_anim_txt, "Animaux");
  lv_obj_set_style_text_color(lbl_anim_txt, COLOR_TEXT_DIM, 0);
  lv_obj_align(lbl_anim_txt, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  // --- Card: Breeding ---
  lv_obj_t *card_breed = create_card(page_home, card_w, 120);
  lv_obj_set_pos(card_breed, margin + card_w + gap, row_y);
  lv_obj_add_flag(card_breed, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card_breed, nav_breeding_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *icon_breed = lv_label_create(card_breed);
  lv_label_set_text(icon_breed, LV_SYMBOL_SHUFFLE);
  lv_obj_set_style_text_font(icon_breed, &lv_font_montserrat_34, 0);
  lv_obj_set_style_text_color(icon_breed, COLOR_LIZARD, 0);
  lv_obj_align(icon_breed, LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t *lbl_breed_count = lv_label_create(card_breed);
  lv_label_set_text_fmt(lbl_breed_count, "%d", breeding_count);
  lv_obj_set_style_text_font(lbl_breed_count, &lv_font_montserrat_34, 0);
  lv_obj_align(lbl_breed_count, LV_ALIGN_BOTTOM_LEFT, 0, -20);

  lv_obj_t *lbl_breed_txt = lv_label_create(card_breed);
  lv_label_set_text(lbl_breed_txt, "Repro");
  lv_obj_set_style_text_color(lbl_breed_txt, COLOR_TEXT_DIM, 0);
  lv_obj_align(lbl_breed_txt, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  // --- Card: Alerts ---
  lv_obj_t *card_alert = create_card(page_home, LCD_H_RES - 40, 100);
  lv_obj_align(card_alert, LV_ALIGN_TOP_MID, 0, row_y + 140);
  lv_obj_add_flag(card_alert, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card_alert, nav_animals_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *icon_alert = lv_label_create(card_alert);
  lv_label_set_text(icon_alert, LV_SYMBOL_BELL);
  lv_obj_set_style_text_font(icon_alert, &lv_font_montserrat_34, 0);
  lv_obj_set_style_text_color(icon_alert, COLOR_DANGER, 0);
  lv_obj_align(icon_alert, LV_ALIGN_LEFT_MID, 10, 0);

  int alerts = reptile_count_feeding_alerts();
  lv_obj_t *lbl_alert = lv_label_create(card_alert);
  if (alerts > 0) {
    lv_label_set_text_fmt(lbl_alert, "%d Animaux a nourrir", alerts);
    lv_obj_set_style_text_color(lbl_alert, COLOR_TEXT, 0);
  } else {
    lv_label_set_text(lbl_alert, "Tout est OK");
    lv_obj_set_style_text_color(lbl_alert, COLOR_SUCCESS, 0);
  }
  lv_obj_set_style_text_font(lbl_alert, &lv_font_montserrat_20, 0);
  lv_obj_align(lbl_alert, LV_ALIGN_LEFT_MID, 60, 0);
}
