#ifndef UI_HOME_H
#define UI_HOME_H

#include "ui_shared.h"

extern lv_obj_t *page_home;

void create_home_page(lv_obj_t *parent);
void create_status_bar(lv_obj_t *parent);
void create_navbar(lv_obj_t *parent);
void update_status_bar(void);

#endif
