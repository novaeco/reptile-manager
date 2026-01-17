/**
 * @file database.h
 * @brief Data Layer for Reptile Manager
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "../models.h"
#include "esp_err.h"

// Limits
#define MAX_REPTILES 30
#define MAX_FEEDINGS 100
#define MAX_HEALTH_RECORDS 50
#define MAX_BREEDINGS 10
#define MAX_INVENTORY_ITEMS 10

// ====================================================================================
// ACCESSORS (GETTERS/SETTERS)
// ====================================================================================

// Reptiles
int db_get_reptile_count(void);
void db_set_reptile_count(int count);
reptile_t *db_get_reptile(int index);
reptile_t *db_get_reptile_by_id(int id);
int db_get_reptile_next_id(void);

// Feedings
int db_get_feeding_count(void);
feeding_record_t *db_get_feeding(int index);
void db_add_feeding(feeding_record_t *record);

// Health
int db_get_health_count(void);
health_record_t *db_get_health(int index);
void db_add_health(health_record_t *record);

// Breeding
int db_get_breeding_count(void);
breeding_record_t *db_get_breeding(int index);
int db_add_breeding(breeding_record_t *record);

// Modifiers (MVC)
void db_update_reptile(int id, reptile_t *data);
void db_delete_reptile(int id);
void db_record_feeding(int id, time_t date, const char *prey, int qty);
void db_record_shed(int id, time_t date);
void db_record_weight(int id, time_t date, int grams);
void db_record_vet_visit(int id, time_t date, const char *notes);

// Helpers

// Inventory
int db_get_inventory_count(void);
void db_set_inventory_count(int count);
inventory_item_t *db_get_inventory_item(int index);

// ====================================================================================
// PERSISTENCE
// ====================================================================================

void db_save_data(void);
void db_load_data(void);
esp_err_t db_export_csv(const char *filepath);
void db_init_demo_data(void);

// ====================================================================================
// EXTERN ACCESS (TEMPORARY FOR REFACTORING)
// ====================================================================================
// Allows main.c to compile before full UI refactoring
extern reptile_t reptiles[MAX_REPTILES];
extern feeding_record_t feedings[MAX_FEEDINGS];
extern health_record_t health_records[MAX_HEALTH_RECORDS];
extern breeding_record_t breedings[MAX_BREEDINGS];
extern inventory_item_t inventory[MAX_INVENTORY_ITEMS];

extern uint8_t reptile_count;
extern uint8_t feeding_count;
extern uint8_t health_record_count;
extern uint8_t breeding_count;
extern uint8_t inventory_count;

// Helpers
const char *db_cites_annex_to_string(cites_annex_t annex);
const char *db_exit_reason_to_string(exit_reason_t reason);
int reptile_days_since_feeding(uint8_t id);
int reptile_count_feeding_alerts(void);

#endif // DATABASE_H
