#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include "ui_shared.h"

extern lv_obj_t *page_settings;
extern lv_obj_t *page_wifi;
extern lv_obj_t *page_bluetooth;

void create_settings_page(lv_obj_t *parent);
void create_wifi_page(lv_obj_t *parent);
void create_bluetooth_page(lv_obj_t *parent);

#endif
