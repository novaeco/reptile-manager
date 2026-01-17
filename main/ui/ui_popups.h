#ifndef UI_POPUPS_H
#define UI_POPUPS_H

#include "ui_shared.h"

void create_popups(void);

// Callbacks exposed for attachment in other pages
void show_feed_popup_cb(lv_event_t *e);
void show_edit_popup_cb(lv_event_t *e);
void show_breeding_popup_cb(lv_event_t *e);
void show_add_health_popup_cb(lv_event_t *e);
void show_history_feed_cb(lv_event_t *e);
void show_history_health_cb(lv_event_t *e);

#endif
