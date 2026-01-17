#include "ui_gallery.h"
#include "data/gallery_manager.h"

lv_obj_t *page_gallery = NULL;
static lv_obj_t *full_img_cont = NULL;

static void close_full_img_cb(lv_event_t *e) {
  if (full_img_cont) {
    lv_obj_delete(full_img_cont);
    full_img_cont = NULL;
  }
}

static void gallery_image_click_cb(lv_event_t *e) {
  lv_obj_t *thumb = lv_event_get_target(e);
  lv_obj_t *label = lv_obj_get_child(thumb, 1);
  if (!label)
    return;

  const char *fname = lv_label_get_text(label);

  full_img_cont = lv_obj_create(lv_layer_top());
  lv_obj_set_size(full_img_cont, LCD_H_RES, LCD_V_RES);
  lv_obj_set_style_bg_color(full_img_cont, lv_color_black(), 0);
  lv_obj_clear_flag(full_img_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *btn_close = lv_button_create(full_img_cont);
  lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_set_size(btn_close, 50, 50);
  lv_obj_set_style_bg_color(btn_close, lv_color_make(200, 50, 50), 0);
  lv_label_set_text(lv_label_create(btn_close), LV_SYMBOL_CLOSE);
  lv_obj_add_event_cb(btn_close, close_full_img_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *l = lv_label_create(full_img_cont);
  lv_label_set_text_fmt(l, "Visualisation: %s", fname);
  lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_set_style_text_color(l, lv_color_white(), 0);

  // Image loading logic
  lv_obj_t *img = lv_image_create(full_img_cont);
  // Use A:/ driver prefix for default stdio mapping if available in
  // esp_lvgl_port path structure matches gallery_manager which returns
  // "/sdcard/imgs/..." If registered letter is 'A', path is
  // "A:/sdcard/imgs/..."? Usually esp_vfs_fat paths are absolute "/sdcard/...".
  // If LVGL FS STDIO is used, we need drive letter.
  // Assuming 'A' or no drive letter if LV_USE_FS_STDIO or LV_USE_FS_POSIX is
  // configured with empty letter. Safest bet for now: "A:%s" if we assume
  // standard drive letter config, OR check if path starts with /

  // Using absolute path from gallery_manager which is "/sdcard/imgs/filename"
  // Try forcing A: prefix as it is common practice in LVGL to separate drivers.
  char path[128];
  snprintf(path, sizeof(path), "A:%s/%s", "/sdcard/imgs", fname);
  lv_image_set_src(img, path);
  lv_obj_center(img);
}

void create_gallery_page(lv_obj_t *parent) {
  page_gallery = lv_obj_create(parent);
  lv_obj_set_size(page_gallery, LCD_H_RES, LCD_V_RES - 110);
  lv_obj_set_pos(page_gallery, 0, 40);
  lv_obj_set_style_bg_color(page_gallery, COLOR_BG_DARK, 0);

  lv_obj_t *lbl = lv_label_create(page_gallery);
  lv_label_set_text(lbl, "Galerie Photos");
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
  lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *list = lv_obj_create(page_gallery);
  lv_obj_set_size(list, LCD_H_RES - 20, LCD_V_RES - 100);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);

  // Use Gallery Manager
  if (!gallery_is_available()) {
    lv_obj_t *err = lv_label_create(list);
    lv_label_set_text(err, "Dossier /imgs introuvable ou SD absente");
    lv_obj_set_style_text_color(err, COLOR_DANGER, 0);
    return;
  }

  gallery_item_t items[20];
  int count = gallery_get_items(items, 20);

  for (int i = 0; i < count; i++) {
    lv_obj_t *thumb = lv_obj_create(list);
    lv_obj_set_size(thumb, 140, 140);
    lv_obj_set_style_bg_color(thumb, COLOR_BG_CARD, 0);

    lv_obj_t *ico = lv_label_create(thumb);
    lv_label_set_text(ico, LV_SYMBOL_IMAGE);
    lv_obj_center(ico);

    lv_obj_t *l = lv_label_create(thumb);
    lv_label_set_text(l, items[i].display_name);
    lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_add_flag(thumb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(thumb, gallery_image_click_cb, LV_EVENT_CLICKED, NULL);
  }
}
