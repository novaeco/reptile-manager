#include "ui_popups.h"
#include "ui_animals.h" // For update_animal_detail/list calls
#include <stdlib.h>
#include <string.h>

// Local Popup Handles
static lv_obj_t *popup_feed = NULL;
static lv_obj_t *popup_edit = NULL;
static lv_obj_t *popup_history = NULL;
static lv_obj_t *popup_breeding = NULL;
static lv_obj_t *popup_health = NULL;
static lv_obj_t *popup_overlay = NULL;

// Widgets for Forms
static lv_obj_t *feed_prey_dd = NULL;
static lv_obj_t *feed_qty_spinbox = NULL;

static lv_obj_t *edit_name_ta = NULL;
static lv_obj_t *edit_weight_ta = NULL;
static lv_obj_t *edit_birth_ta = NULL;
static lv_obj_t *edit_sex_dd = NULL;
static lv_obj_t *edit_chip_ta = NULL;
static lv_obj_t *edit_morph_ta = NULL;
static lv_obj_t *edit_cites_dd = NULL;
static lv_obj_t *edit_origin_ta = NULL;

static lv_obj_t *edit_breed_male_dd = NULL;
// static lv_obj_t *edit_breed_female_dd = NULL; // Unused
static int edit_breeding_id = -1;

static lv_obj_t *health_type_dd = NULL;
static lv_obj_t *health_desc_ta = NULL;

static lv_obj_t *list_history = NULL;

// Helper: Keyboard handling
static lv_obj_t *ui_keyboard = NULL;
static void ta_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);
  if (code == LV_EVENT_FOCUSED) {
    if (!ui_keyboard) {
      ui_keyboard = lv_keyboard_create(lv_layer_top());
      lv_obj_add_flag(ui_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    lv_keyboard_set_textarea(ui_keyboard, ta);
    lv_obj_clear_flag(ui_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui_keyboard);
  } else if (code == LV_EVENT_DEFOCUSED) {
    if (ui_keyboard) {
      lv_keyboard_set_textarea(ui_keyboard, NULL);
      lv_obj_add_flag(ui_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void close_popup_cb(lv_event_t *e) {
  if (popup_feed)
    lv_obj_add_flag(popup_feed, LV_OBJ_FLAG_HIDDEN);
  if (popup_edit)
    lv_obj_add_flag(popup_edit, LV_OBJ_FLAG_HIDDEN);
  if (popup_history)
    lv_obj_add_flag(popup_history, LV_OBJ_FLAG_HIDDEN);
  if (popup_breeding)
    lv_obj_add_flag(popup_breeding, LV_OBJ_FLAG_HIDDEN);
  if (popup_health)
    lv_obj_add_flag(popup_health, LV_OBJ_FLAG_HIDDEN);
  if (popup_overlay)
    lv_obj_add_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);

  if (ui_keyboard)
    lv_obj_add_flag(ui_keyboard, LV_OBJ_FLAG_HIDDEN);
}

static void save_feeding_cb(lv_event_t *e) {
  if (selected_animal_id >= 0 && selected_animal_id < reptile_count) {
    char buf[32];
    lv_dropdown_get_selected_str(feed_prey_dd, buf, sizeof(buf));
    int qty = lv_spinbox_get_value(feed_qty_spinbox);

    db_record_feeding(selected_animal_id, time(NULL), buf, qty);

    show_toast("Repas Enregistre", COLOR_SUCCESS);
    update_animal_detail();
    update_animal_list();
  }
  close_popup_cb(NULL);
}

// Spinbox
// static void feed_qty_inc_cb(lv_event_t *e) {
//   lv_spinbox_increment(feed_qty_spinbox);
// }
// static void feed_qty_dec_cb(lv_event_t *e) {
//   lv_spinbox_decrement(feed_qty_spinbox);
// }

static void edit_animal_delete_cb(lv_event_t *e) {
  if (selected_animal_id >= 0) {
    db_delete_reptile(selected_animal_id);
    update_animal_list();
    navigate_to(PAGE_ANIMALS); // Go back to list
    show_toast("Animal supprime", COLOR_DANGER);
  }
  close_popup_cb(NULL);
}

static void save_edit_cb(lv_event_t *e) {
  // Logic from ui_manager.c
  // Validate
  const char *txt_name = lv_textarea_get_text(edit_name_ta);
  if (strlen(txt_name) == 0) {
    show_toast("Nom requis", COLOR_DANGER);
    return;
  }

  reptile_t data;
  // If editing existing, copy first to preserve un-edited fields?
  // STRICT MVC: We should ask DB for current data, modify it, then send back
  // via update. We included database.h in ui_popups via ui_shared? ui_shared
  // includes models.h. database.h includes models.h. ui_popups includes
  // ui_shared. We need database.h access.
  if (selected_animal_id >= 0) {
    reptile_t *current = db_get_reptile(selected_animal_id);
    if (current)
      data = *current;
    else
      strncpy(data.name, "Unknown", sizeof(data.name) - 1);
  } else {
    memset(&data, 0, sizeof(reptile_t));
    data.active = true;
    data.species = SPECIES_SNAKE; // Default
  }

  // Update fields from UI
  strncpy(data.name, txt_name, sizeof(data.name) - 1);

  data.weight_grams = atoi(lv_textarea_get_text(edit_weight_ta));
  data.birth_year = atoi(lv_textarea_get_text(edit_birth_ta));
  data.sex = (reptile_sex_t)lv_dropdown_get_selected(edit_sex_dd);

  strncpy(data.microchip, lv_textarea_get_text(edit_chip_ta),
          sizeof(data.microchip) - 1);
  strncpy(data.morph, lv_textarea_get_text(edit_morph_ta),
          sizeof(data.morph) - 1);
  strncpy(data.origin, lv_textarea_get_text(edit_origin_ta),
          sizeof(data.origin) - 1);

  data.cites_annex = (cites_annex_t)lv_dropdown_get_selected(edit_cites_dd);

  db_update_reptile(selected_animal_id, &data);

  if (selected_animal_id == -1) {
    update_animal_list();
    // create_animals_page(lv_screen_active()); // No, list update is enough
    // usually
    show_toast("Animal Ajoute", COLOR_SUCCESS);
  } else {
    update_animal_detail();
    update_animal_list();
    show_toast("Modifie", COLOR_SUCCESS);
  }
  close_popup_cb(NULL);
}

static void save_health_cb(lv_event_t *e) {
  if (selected_animal_id >= 0) {
    char buf[32];
    lv_dropdown_get_selected_str(health_type_dd, buf, sizeof(buf));
    if (strcmp(buf, "Mue") == 0) {
      db_record_shed(selected_animal_id, time(NULL));
    } else if (strcmp(buf, "Pesee") == 0) {
      // Example handling
      db_record_weight(selected_animal_id, time(NULL),
                       0); // Need input for weight?
    }
    update_animal_detail();
    show_toast("Sante enregistree", COLOR_SUCCESS);
  }
  close_popup_cb(NULL);
}

static void save_breeding_cb(lv_event_t *e) {
  // Breeding save logic
  db_save_data();
  create_breeding_page(lv_screen_active()); // Refresh page
  close_popup_cb(NULL);
}

void create_popups(void) {
  // Dimmed Overlay
  popup_overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(popup_overlay, LCD_H_RES, LCD_V_RES);
  lv_obj_set_style_bg_color(popup_overlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(popup_overlay, LV_OPA_50, 0);
  lv_obj_add_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(popup_overlay, close_popup_cb, LV_EVENT_CLICKED, NULL);

  // --- FEEDING POPUP ---
  popup_feed = lv_obj_create(lv_layer_top());
  lv_obj_set_size(popup_feed, 340, 320);
  lv_obj_center(popup_feed);
  lv_obj_set_style_bg_color(popup_feed, COLOR_BG_CARD, 0);
  lv_obj_add_flag(popup_feed, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *lbl = lv_label_create(popup_feed);
  lv_label_set_text(lbl, "Nouveau Repas");
  lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

  // Dropdown
  feed_prey_dd = lv_dropdown_create(popup_feed);
  lv_dropdown_set_options(feed_prey_dd, "Souris\nRat\nGrillon");
  lv_obj_align(feed_prey_dd, LV_ALIGN_TOP_RIGHT, 0, 45);

  // Spinbox
  feed_qty_spinbox = lv_spinbox_create(popup_feed);
  lv_spinbox_set_range(feed_qty_spinbox, 1, 10);
  lv_spinbox_set_digit_format(feed_qty_spinbox, 2, 0);
  lv_spinbox_set_step(feed_qty_spinbox, 1);
  lv_obj_align(feed_qty_spinbox, LV_ALIGN_TOP_RIGHT, -60, 95);

  // Buttons
  lv_obj_t *btn_save = lv_button_create(popup_feed);
  lv_label_set_text(lv_label_create(btn_save), "Valider");
  lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(btn_save, save_feeding_cb, LV_EVENT_CLICKED, NULL);

  // --- EDIT POPUP ---
  popup_edit = lv_obj_create(lv_layer_top());
  lv_obj_set_size(popup_edit, 360, 450);
  lv_obj_center(popup_edit);
  lv_obj_set_style_bg_color(popup_edit, COLOR_BG_CARD, 0);
  lv_obj_add_flag(popup_edit, LV_OBJ_FLAG_HIDDEN);

  edit_name_ta = lv_textarea_create(popup_edit);
  lv_textarea_set_one_line(edit_name_ta, true);
  lv_textarea_set_placeholder_text(edit_name_ta, "Nom");
  lv_obj_add_event_cb(edit_name_ta, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(edit_name_ta, LV_ALIGN_TOP_LEFT, 0, 75);

  edit_weight_ta = lv_textarea_create(popup_edit);
  lv_textarea_set_one_line(edit_weight_ta, true);
  lv_textarea_set_placeholder_text(edit_weight_ta, "Poids (g)");
  lv_obj_add_event_cb(edit_weight_ta, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(edit_weight_ta, LV_ALIGN_TOP_RIGHT, 0, 75);

  edit_birth_ta = lv_textarea_create(popup_edit);
  lv_textarea_set_one_line(edit_birth_ta, true);
  lv_textarea_set_placeholder_text(edit_birth_ta, "Annee Naissan.");
  lv_obj_add_event_cb(edit_birth_ta, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(edit_birth_ta, LV_ALIGN_TOP_LEFT, 0, 120);

  edit_sex_dd = lv_dropdown_create(popup_edit);
  lv_dropdown_set_options(edit_sex_dd, "Inconnu\nMale\nFemelle");
  lv_obj_align(edit_sex_dd, LV_ALIGN_TOP_RIGHT, 0, 120);

  edit_chip_ta = lv_textarea_create(popup_edit);
  lv_textarea_set_one_line(edit_chip_ta, true);
  lv_textarea_set_placeholder_text(edit_chip_ta, "Puce/Id");
  lv_obj_add_event_cb(edit_chip_ta, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(edit_chip_ta, LV_ALIGN_TOP_LEFT, 0, 165);

  edit_morph_ta = lv_textarea_create(popup_edit);
  lv_textarea_set_one_line(edit_morph_ta, true);
  lv_textarea_set_placeholder_text(edit_morph_ta, "Phase/Morph");
  lv_obj_add_event_cb(edit_morph_ta, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(edit_morph_ta, LV_ALIGN_TOP_RIGHT, 0, 165);

  edit_cites_dd = lv_dropdown_create(popup_edit);
  lv_dropdown_set_options(edit_cites_dd,
                          "NC\nAnnexe A\nAnnexe B\nAnnexe C\nAnnexe D");
  lv_obj_align(edit_cites_dd, LV_ALIGN_TOP_LEFT, 0, 210);

  edit_origin_ta = lv_textarea_create(popup_edit);
  lv_textarea_set_one_line(edit_origin_ta, true);
  lv_textarea_set_placeholder_text(edit_origin_ta, "Origine");
  lv_obj_add_event_cb(edit_origin_ta, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(edit_origin_ta, LV_ALIGN_TOP_RIGHT, 0, 210);

  lv_obj_t *btn_edit_save = lv_button_create(popup_edit);
  lv_label_set_text(lv_label_create(btn_edit_save), "Sauver");
  lv_obj_align(btn_edit_save, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(btn_edit_save, save_edit_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btn_edit_del = lv_button_create(popup_edit);
  lv_obj_set_style_bg_color(btn_edit_del, COLOR_DANGER, 0);
  lv_label_set_text(lv_label_create(btn_edit_del), "Suppr.");
  lv_obj_align(btn_edit_del, LV_ALIGN_BOTTOM_MID, 0, -60);
  lv_obj_add_event_cb(btn_edit_del, edit_animal_delete_cb, LV_EVENT_CLICKED,
                      NULL);

  // --- HEALTH POPUP ---
  popup_health = lv_obj_create(lv_layer_top());
  lv_obj_set_size(popup_health, 340, 350);
  lv_obj_center(popup_health);
  lv_obj_set_style_bg_color(popup_health, COLOR_BG_CARD, 0);
  lv_obj_add_flag(popup_health, LV_OBJ_FLAG_HIDDEN);

  health_type_dd = lv_dropdown_create(popup_health);
  lv_dropdown_set_options(health_type_dd, "Mue\nPesee\nVeto");
  lv_obj_align(health_type_dd, LV_ALIGN_TOP_LEFT, 10, 75);

  health_desc_ta = lv_textarea_create(popup_health);
  lv_obj_set_size(health_desc_ta, 300, 100);
  lv_obj_align(health_desc_ta, LV_ALIGN_TOP_LEFT, 10, 145);
  lv_obj_add_event_cb(health_desc_ta, ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *btn_hl_save = lv_button_create(popup_health);
  lv_label_set_text(lv_label_create(btn_hl_save), "Valider");
  lv_obj_align(btn_hl_save, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(btn_hl_save, save_health_cb, LV_EVENT_CLICKED, NULL);

  // --- BREEDING POPUP ---
  popup_breeding = lv_obj_create(lv_layer_top());
  lv_obj_set_size(popup_breeding, 340, 350);
  lv_obj_center(popup_breeding);
  lv_obj_set_style_bg_color(popup_breeding, COLOR_BG_CARD, 0);
  lv_obj_add_flag(popup_breeding, LV_OBJ_FLAG_HIDDEN);

  edit_breed_male_dd = lv_dropdown_create(popup_breeding);
  lv_obj_align(edit_breed_male_dd, LV_ALIGN_TOP_LEFT, 10, 75);

  lv_obj_t *btn_br_save = lv_button_create(popup_breeding);
  lv_label_set_text(lv_label_create(btn_br_save), "Valider");
  lv_obj_align(btn_br_save, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(btn_br_save, save_breeding_cb, LV_EVENT_CLICKED, NULL);

  // --- HISTORY POPUP ---
  popup_history = lv_obj_create(lv_layer_top());
  lv_obj_set_size(popup_history, 360, 500);
  lv_obj_center(popup_history);
  lv_obj_set_style_bg_color(popup_history, COLOR_BG_CARD, 0);
  lv_obj_add_flag(popup_history, LV_OBJ_FLAG_HIDDEN);

  list_history = lv_obj_create(popup_history);
  lv_obj_set_size(list_history, 320, 380);
  lv_obj_align(list_history, LV_ALIGN_TOP_MID, 0, 40);

  lv_obj_t *btn_hist_close = lv_button_create(popup_history);
  lv_label_set_text(lv_label_create(btn_hist_close), "Fermer");
  lv_obj_align(btn_hist_close, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_event_cb(btn_hist_close, close_popup_cb, LV_EVENT_CLICKED, NULL);
}

void show_feed_popup_cb(lv_event_t *e) {
  if (!popup_feed)
    create_popups();
  lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(popup_feed, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(popup_overlay);
  lv_obj_move_foreground(popup_feed);
}

void show_edit_popup_cb(lv_event_t *e) {
  if (!popup_edit)
    create_popups();

  // Fill data if selected_animal_id >= 0
  if (selected_animal_id >= 0) {
    lv_textarea_set_text(edit_name_ta, reptiles[selected_animal_id].name);
    // ...
  } else {
    lv_textarea_set_text(edit_name_ta, "");
  }

  lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(popup_edit, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(popup_overlay);
  lv_obj_move_foreground(popup_edit);
}

void show_breeding_popup_cb(lv_event_t *e) {
  if (!popup_breeding)
    create_popups();

  edit_breeding_id = (int)(intptr_t)lv_event_get_user_data(e);

  // Populate dropdowns from reptiles array
  lv_dropdown_clear_options(edit_breed_male_dd);
  for (int i = 0; i < reptile_count; i++) {
    if (reptiles[i].sex == SEX_MALE)
      lv_dropdown_add_option(edit_breed_male_dd, reptiles[i].name,
                             LV_DROPDOWN_POS_LAST);
  }

  lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(popup_breeding, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(popup_overlay);
  lv_obj_move_foreground(popup_breeding);
}

void show_add_health_popup_cb(lv_event_t *e) {
  if (!popup_health)
    create_popups();
  lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(popup_health, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(popup_overlay);
  lv_obj_move_foreground(popup_health);
}

void show_history_feed_cb(lv_event_t *e) {
  if (!popup_history)
    create_popups();
  // Populate list
  lv_obj_clean(list_history);
  // Loop stored feedings

  lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(popup_history, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(popup_overlay);
  lv_obj_move_foreground(popup_history);
}

void show_history_health_cb(lv_event_t *e) {
  if (!popup_history)
    create_popups();
  lv_obj_clean(list_history);

  lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(popup_history, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(popup_overlay);
  lv_obj_move_foreground(popup_history);
}
