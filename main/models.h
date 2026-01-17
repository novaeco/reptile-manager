/**
 * @file models.h
 * @brief Data models for Reptile Manager
 */

#ifndef MODELS_H
#define MODELS_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ====================================================================================
// REPTILE MANAGER DATA STRUCTURES
// ====================================================================================

// Animal species types
typedef enum {
  SPECIES_SNAKE = 0,
  SPECIES_LIZARD,
  SPECIES_TURTLE,
  SPECIES_OTHER
} reptile_species_t;

// Animal sex
typedef enum { SEX_UNKNOWN = 0, SEX_MALE, SEX_FEMALE } reptile_sex_t;

// Health status
typedef enum { HEALTH_GOOD = 0, HEALTH_ATTENTION, HEALTH_SICK } health_status_t;

// CITES Annex classification (Règlement UE 338/97)
typedef enum {
  CITES_NOT_LISTED = 0, // Non concerné
  CITES_ANNEX_A,        // Annexe A (CITES I) - Commerce interdit
  CITES_ANNEX_B,        // Annexe B (CITES II) - Commerce régulé
  CITES_ANNEX_C,        // Annexe C (CITES III) - Surveillance
  CITES_ANNEX_D         // Annexe D - Suivi statistique
} cites_annex_t;

// Exit/Sortie reason
typedef enum {
  EXIT_NONE = 0,   // Still in collection
  EXIT_SOLD,       // Vendu
  EXIT_DONATED,    // Donné
  EXIT_DECEASED,   // Décédé
  EXIT_ESCAPED,    // Évasion
  EXIT_CONFISCATED // Confisqué
} exit_reason_t;

// Animal record structure - CONFORME Arrêté 10 août 2004
typedef struct {
  // === Identification unique ===
  uint8_t id;
  char uuid[37]; // UUID v4 format (36 chars + null)

  // === Identification espèce ===
  char name[32];               // Nom usuel
  char species_common[48];     // Nom vernaculaire (ex: "Python Royal")
  char species_scientific[64]; // Nom latin (ex: "Python regius")
  char morph[32];              // Phase/mutation (ex: "Pastel Banana")
  reptile_species_t species;
  reptile_sex_t sex;

  // === Identification individuelle ===
  char microchip[20];   // N° puce ISO 11784/11785 (15 digits)
  char ring_number[16]; // N° bague si applicable

  // === Naissance ===
  uint16_t birth_year;
  uint8_t birth_month;
  uint8_t birth_day;
  bool birth_estimated; // Date estimée vs confirmée

  // === CITES / Réglementation ===
  cites_annex_t cites_annex;
  char cites_permit[32]; // N° permis CITES si annexe A
  char cites_date[16];   // Date du permis (YYYY-MM-DD)
  bool cdc_required;     // Nécessite certificat de capacité

  // === Acquisition/Entrée ===
  time_t date_acquisition;   // Date d'entrée dans l'élevage
  char origin[64];           // Provenance (Élevage X, Animalerie Y, Import)
  char origin_country[3];    // Code pays ISO 3166-1 alpha-2
  char breeder_name[64];     // Nom éleveur/vendeur
  char breeder_address[128]; // Adresse complète
  char breeder_cdc[32];      // N° CDC vendeur si applicable
  bool captive_bred;         // Né en captivité (NC) vs prélevé (W)

  // === Sortie/Cession ===
  time_t date_exit; // Date de sortie (0 si encore présent)
  exit_reason_t exit_reason;
  char recipient_name[64];     // Nom acquéreur/cessionnaire
  char recipient_address[128]; // Adresse acquéreur
  uint16_t sale_price;         // Prix de vente (0 si don)

  // === Données techniques ===
  uint16_t weight_grams; // Poids actuel en grammes
  uint8_t terrarium_id;
  uint16_t purchase_price; // Prix d'achat en euros
  time_t last_feeding;
  time_t last_weight;
  time_t last_shed;
  health_status_t health;
  bool is_breeding;
  char photo_path[64];
  char notes[128];

  // Documents
  bool doc_cession_ok; // Certificat de Cession
  bool doc_entree_ok;  // Bon d'entrée (Livre de Police)

  bool active; // false = sorti du cheptel
} reptile_t;

// Feeding record
typedef struct {
  uint8_t animal_id;
  time_t timestamp;
  char prey_type[24]; // e.g., "Souris adulte", "Grillon"
  uint8_t prey_count;
  bool accepted; // Did animal eat?
} feeding_record_t;

// Health/Vet record
typedef struct {
  uint8_t animal_id;
  time_t timestamp;
  char event_type[24]; // "Vermifuge", "Mue", "Vétérinaire"
  char description[64];
  uint16_t weight_grams; // Weight at time of event
} health_record_t;

// Breeding/Reproduction record
typedef struct {
  uint8_t id;
  uint8_t female_id;
  uint8_t male_id;
  time_t pairing_date;
  time_t laying_date; // Actual or estimated
  uint8_t egg_count;
  time_t hatch_date; // Actual or estimated (laying + 60 days typical)
  uint8_t hatched_count;
  bool active;
} breeding_record_t;

// Inventory item
typedef struct {
  char name[24];
  uint16_t quantity;
  uint16_t alert_threshold; // Alert when below this
  char unit[8];             // "pcs", "kg", etc.
} inventory_item_t;

#endif // MODELS_H
