/**
 * @file database.c
 * @brief Implementation of Data Layer
 */

#include "database.h"
#include "../ui_theme.h" // For colors if needed, or remove if decoupling strict
#include "esp_err.h"
#include "esp_log.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static const char *TAG = "DATABASE";

// ====================================================================================
// DATA STORAGE (Moved from main.c)
// ====================================================================================

reptile_t reptiles[MAX_REPTILES];
feeding_record_t feedings[MAX_FEEDINGS];
health_record_t health_records[MAX_HEALTH_RECORDS];
breeding_record_t breedings[MAX_BREEDINGS];
inventory_item_t inventory[MAX_INVENTORY_ITEMS];

uint8_t reptile_count = 0;
uint8_t feeding_count = 0;
uint8_t health_record_count = 0;
uint8_t breeding_count = 0;
uint8_t inventory_count = 0;

// ====================================================================================
// ACCESSORS
// ====================================================================================

int db_get_reptile_count(void) { return reptile_count; }
void db_set_reptile_count(int count) { reptile_count = count; }

reptile_t *db_get_reptile(int index) {
  if (index >= 0 && index < MAX_REPTILES)
    return &reptiles[index];
  return NULL;
}

reptile_t *db_get_reptile_by_id(int id) {
  for (int i = 0; i < reptile_count; i++) {
    if (reptiles[i].id == id)
      return &reptiles[i];
  }
  return NULL;
}

int db_get_reptile_next_id(void) {
  int max = 0;
  for (int i = 0; i < reptile_count; i++) {
    if (reptiles[i].id > max)
      max = reptiles[i].id;
  }
  return max + 1;
}

int db_get_feeding_count(void) { return feeding_count; }
feeding_record_t *db_get_feeding(int index) {
  if (index >= 0 && index < MAX_FEEDINGS)
    return &feedings[index];
  return NULL;
}
void db_add_feeding(feeding_record_t *record) {
  if (feeding_count < MAX_FEEDINGS) {
    feedings[feeding_count] = *record;
    feeding_count++;
  }
}

int db_get_health_count(void) { return health_record_count; }
health_record_t *db_get_health(int index) {
  if (index >= 0 && index < MAX_HEALTH_RECORDS)
    return &health_records[index];
  return NULL;
}
void db_add_health(health_record_t *record) {
  if (health_record_count < MAX_HEALTH_RECORDS) {
    health_records[health_record_count] = *record;
    health_record_count++;
  }
}

int db_get_breeding_count(void) { return breeding_count; }
breeding_record_t *db_get_breeding(int index) {
  if (index >= 0 && index < MAX_BREEDINGS)
    return &breedings[index];
  return NULL;
}
int db_add_breeding(breeding_record_t *record) {
  if (breeding_count < MAX_BREEDINGS) {
    breedings[breeding_count] = *record;
    breeding_count++;
    return breeding_count - 1;
  }
  return -1;
}

int db_get_inventory_count(void) { return inventory_count; }
void db_set_inventory_count(int count) { inventory_count = count; }
inventory_item_t *db_get_inventory_item(int index) {
  if (index >= 0 && index < MAX_INVENTORY_ITEMS)
    return &inventory[index];
  return NULL;
}

// ====================================================================================
// PERSISTENCE
// ====================================================================================

#define DATA_FILE_PATH "/sdcard/reptile_data.bin"

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint8_t reptile_count;
  uint8_t feeding_count;
  uint8_t health_count;
  uint8_t breeding_count;
  uint8_t inventory_count;
} data_header_t;

static const uint32_t DATA_MAGIC = 0x52455054; // "REPT"
static const uint32_t DATA_VERSION = 1;

// Stub for toast since UI is not here
// In full refactor, UI observes Data changes, or Controller calls Data then UI.
// For now, we will assume save is called from UI which handles Toast.
// Or we print log.

void db_save_data(void) {
  // Check if SD mounted? We assume main.c checks or we check global flag if
  // accessible. For strict separation, we should try open.
  FILE *f = fopen(DATA_FILE_PATH, "wb");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open data file for writing");
    return;
  }

  data_header_t header = {.magic = DATA_MAGIC,
                          .version = DATA_VERSION,
                          .reptile_count = reptile_count,
                          .feeding_count = feeding_count,
                          .health_count = health_record_count,
                          .breeding_count = breeding_count,
                          .inventory_count = inventory_count};
  fwrite(&header, sizeof(data_header_t), 1, f);

  if (reptile_count > 0)
    fwrite(reptiles, sizeof(reptile_t), reptile_count, f);
  if (feeding_count > 0)
    fwrite(feedings, sizeof(feeding_record_t), feeding_count, f);
  if (health_record_count > 0)
    fwrite(health_records, sizeof(health_record_t), health_record_count, f);
  if (breeding_count > 0)
    fwrite(breedings, sizeof(breeding_record_t), breeding_count, f);
  if (inventory_count > 0)
    fwrite(inventory, sizeof(inventory_item_t), inventory_count, f);

  fclose(f);
  ESP_LOGI(TAG, "Data saved successfully to %s", DATA_FILE_PATH);
}

void db_init_demo_data(void) {
  ESP_LOGI(TAG, "Initializing DEMO data...");
  reptile_count = 0;

  // 1. Python Royal
  reptiles[0].id = 1;
  strcpy(reptiles[0].name, "Nagini");
  strcpy(reptiles[0].species_common, "Python Royal");
  strcpy(reptiles[0].species_scientific, "Python regius");
  strcpy(reptiles[0].morph, "Banana Pastel");
  reptiles[0].species = SPECIES_SNAKE;
  reptiles[0].sex = SEX_MALE;
  reptiles[0].birth_year = 2022;
  reptiles[0].weight_grams = 450;
  reptiles[0].active = true;
  strcpy(reptiles[0].microchip, "250269600123456");
  reptiles[0].cites_annex = CITES_ANNEX_B;
  reptiles[0].last_feeding = time(NULL) - (5 * 24 * 3600);
  reptile_count++;

  // 2. Gecko
  reptiles[1].id = 2;
  strcpy(reptiles[1].name, "Yoshi");
  strcpy(reptiles[1].species_common, "Gecko Leo");
  strcpy(reptiles[1].species_scientific, "Eublepharis m.");
  strcpy(reptiles[1].morph, "Tangerine");
  reptiles[1].species = SPECIES_LIZARD;
  reptiles[1].sex = SEX_FEMALE;
  reptiles[1].birth_year = 2021;
  reptiles[1].weight_grams = 65;
  reptiles[1].active = true;
  reptiles[1].cites_annex = CITES_NOT_LISTED;
  reptiles[1].last_feeding = time(NULL) - (2 * 24 * 3600);
  reptile_count++;

  // 3. Boa
  reptiles[2].id = 3;
  strcpy(reptiles[2].name, "Kaa");
  strcpy(reptiles[2].species_common, "Boa Constrictor");
  strcpy(reptiles[2].species_scientific, "Boa imperator");
  strcpy(reptiles[2].morph, "Hypo Jungle");
  reptiles[2].species = SPECIES_SNAKE;
  reptiles[2].sex = SEX_FEMALE;
  reptiles[2].birth_year = 2019;
  reptiles[2].weight_grams = 2100;
  reptiles[2].active = true;
  strcpy(reptiles[2].microchip, "250269600987654");
  reptiles[2].cites_annex = CITES_ANNEX_B;
  reptiles[2].last_feeding = time(NULL) - (10 * 24 * 3600);
  reptile_count++;
}

void db_load_data(void) {
  FILE *f = fopen(DATA_FILE_PATH, "rb");
  if (f == NULL) {
    ESP_LOGW(TAG, "No saved data found, using defaults");
    db_init_demo_data();
    return;
  }

  data_header_t header;
  fread(&header, sizeof(data_header_t), 1, f);

  if (header.magic != DATA_MAGIC) {
    ESP_LOGE(TAG, "Invalid data file format");
    fclose(f);
    db_init_demo_data();
    return;
  }

  reptile_count = header.reptile_count;
  feeding_count = header.feeding_count;
  health_record_count = header.health_count;
  breeding_count = header.breeding_count;
  inventory_count = header.inventory_count;

  if (reptile_count > 0)
    fread(reptiles, sizeof(reptile_t), reptile_count, f);
  if (feeding_count > 0)
    fread(feedings, sizeof(feeding_record_t), feeding_count, f);
  if (health_record_count > 0)
    fread(health_records, sizeof(health_record_t), health_record_count, f);
  if (breeding_count > 0)
    fread(breedings, sizeof(breeding_record_t), breeding_count, f);
  if (inventory_count > 0)
    fread(inventory, sizeof(inventory_item_t), inventory_count, f);

  fclose(f);
  ESP_LOGI(TAG, "Data loaded safely. %d reptiles.", reptile_count);
}

// ====================================================================================
// HELPERS
// ====================================================================================

const char *db_cites_annex_to_string(cites_annex_t annex) {
  switch (annex) {
  case CITES_ANNEX_A:
    return "A";
  case CITES_ANNEX_B:
    return "B";
  case CITES_ANNEX_C:
    return "C";
  case CITES_ANNEX_D:
    return "D";
  default:
    return "Non listé";
  }
}

const char *db_exit_reason_to_string(exit_reason_t reason) {
  switch (reason) {
  case EXIT_SOLD:
    return "Vente";
  case EXIT_DONATED:
    return "Don";
  case EXIT_DECEASED:
    return "Décès";
  case EXIT_ESCAPED:
    return "Évasion";
  case EXIT_CONFISCATED:
    return "Confiscation";
  default:
    return "";
  }
}

static void format_date(time_t timestamp, char *buf, size_t len) {
  if (timestamp == 0) {
    buf[0] = '\0';
    return;
  }
  struct tm *tm_info = localtime(&timestamp);
  strftime(buf, len, "%Y-%m-%d", tm_info);
}

esp_err_t db_export_csv(const char *filepath) {
  ESP_LOGI(TAG, "Exporting registre to CSV: %s", filepath);

  FILE *f = fopen(filepath, "w");
  if (!f) {
    ESP_LOGE(TAG, "Failed to create CSV file: %s", filepath);
    return ESP_ERR_NOT_FOUND;
  }

  // CSV Header
  fprintf(f,
          "ID,UUID,Nom,Espece_Commune,Espece_Scientifique,Identification,Sexe,"
          "Date_Naissance,Naissance_Estimee,CITES_Annexe,CITES_Permis,"
          "Date_Entree,Provenance,Pays_Origine,Eleveur_Nom,Ne_Captivite,"
          "Date_Sortie,Motif_Sortie,Destinataire_Nom,Destinataire_Adresse,"
          "Poids_Grammes,Actif\n");

  char date_birth[16], date_acq[16], date_exit[16];

  for (int i = 0; i < reptile_count; i++) {
    reptile_t *r = &reptiles[i];

    if (r->birth_year > 0) {
      snprintf(date_birth, sizeof(date_birth), "%04d-%02d-%02d", r->birth_year,
               r->birth_month ? r->birth_month : 1,
               r->birth_day ? r->birth_day : 1);
    } else {
      date_birth[0] = '\0';
    }
    format_date(r->date_acquisition, date_acq, sizeof(date_acq));
    format_date(r->date_exit, date_exit, sizeof(date_exit));

    fprintf(f,
            "%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%s,"
            "%s,%s,%s,\"%s\","
            "%s,\"%s\",\"%s\",\"%s\",%s,"
            "%s,%s,\"%s\",\"%s\","
            "%d,%s\n",
            r->id, r->uuid, r->name, r->species_common, r->species_scientific,
            r->microchip,
            (r->sex == SEX_MALE)     ? "M"
            : (r->sex == SEX_FEMALE) ? "F"
                                     : "?",
            date_birth, r->birth_estimated ? "Oui" : "Non",
            db_cites_annex_to_string(r->cites_annex), r->cites_permit, date_acq,
            r->origin, r->origin_country, r->breeder_name,
            r->captive_bred ? "Oui" : "Non", date_exit,
            db_exit_reason_to_string(r->exit_reason), r->recipient_name,
            r->recipient_address, r->weight_grams, r->active ? "Oui" : "Non");
  }

  fclose(f);
  ESP_LOGI(TAG, "Registre exported: %d animals to %s", reptile_count, filepath);
  return ESP_OK;
}

// ====================================================================================
// MODIFIERS (MVC)
// ====================================================================================

void db_update_reptile(int id, reptile_t *data) {
  if (id >= 0 && id < MAX_REPTILES) {
    if (id >= reptile_count) {
      // Adding new
      reptiles[reptile_count] = *data;
      reptiles[reptile_count].id = reptile_count + 1; // Basic ID gen
      reptile_count++;
    } else {
      // Updating existing
      // Preserve ID? Or assume *data has it.
      // We generally assume *data has proper content, but ID should be
      // immutable or strictly managed. Copy content
      reptiles[id] = *data;
    }
    db_save_data();
  }
}

void db_delete_reptile(int id) {
  if (id >= 0 && id < reptile_count) {
    reptiles[id].active = false; // Soft delete
    db_save_data();
  }
}

void db_record_feeding(int id, time_t date, const char *prey, int qty) {
  if (id >= 0 && id < reptile_count) {
    reptiles[id].last_feeding = date;

    // Add history record (if space)
    if (feeding_count < MAX_FEEDINGS) {
      feedings[feeding_count].animal_id = reptiles[id].id;
      feedings[feeding_count].timestamp = date;
      if (prey) {
        strncpy(feedings[feeding_count].prey_type, prey,
                sizeof(feedings[feeding_count].prey_type) - 1);
      } else {
        feedings[feeding_count].prey_type[0] = '\0';
      }
      feedings[feeding_count].prey_count = (uint8_t)qty;
      feedings[feeding_count].accepted = true;
      feeding_count++;
    }
    db_save_data();
  }
}

void db_record_shed(int id, time_t date) {
  if (id >= 0 && id < reptile_count) {
    reptiles[id].last_shed = date;
    db_save_data();
  }
}

void db_record_weight(int id, time_t date, int grams) {
  if (id >= 0 && id < reptile_count) {
    reptiles[id].weight_grams = grams;
    db_save_data();
  }
}

void db_record_vet_visit(int id, time_t date, const char *notes) {
  // Basic placeholder
  db_save_data();
}

// ====================================================================================
// LOGIC HELPERS
// ====================================================================================

int reptile_days_since_feeding(uint8_t id) {
  if (id >= reptile_count || reptiles[id].last_feeding == 0)
    return -1;
  time_t now = time(NULL);
  return (now - reptiles[id].last_feeding) / (24 * 3600);
}

int reptile_count_feeding_alerts(void) {
  int count = 0;
  for (int i = 0; i < reptile_count; i++) {
    if (!reptiles[i].active)
      continue;
    int days = reptile_days_since_feeding(i);
    int threshold = (reptiles[i].species == SPECIES_SNAKE) ? 7 : 3;
    if (days >= threshold)
      count++;
  }
  return count;
}
