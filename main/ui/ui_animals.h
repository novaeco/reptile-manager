#ifndef UI_ANIMALS_H
#define UI_ANIMALS_H

#include "ui_shared.h"

extern lv_obj_t *page_animals;
extern lv_obj_t *page_animal_detail;
extern lv_obj_t *page_breeding;
extern lv_obj_t *page_conformity;

void create_animals_page(lv_obj_t *parent);
void create_animal_detail_page(lv_obj_t *parent);
void create_breeding_page(lv_obj_t *parent);
void create_conformity_page(lv_obj_t *parent);

void update_animal_list(void);
void update_animal_detail(void);

#endif
