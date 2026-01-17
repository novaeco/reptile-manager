#ifndef GALLERY_MANAGER_H
#define GALLERY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_GALLERY_PATH 270
#define MAX_GALLERY_NAME 64

typedef struct {
  char full_path[MAX_GALLERY_PATH];
  char display_name[MAX_GALLERY_NAME];
} gallery_item_t;

/**
 * @brief Check if gallery source (SD card) is available
 */
bool gallery_is_available(void);

/**
 * @brief Get list of images in gallery
 * @param items Array to fill
 * @param max_count Maximum number of items
 * @return Number of items found
 */
int gallery_get_items(gallery_item_t *items, int max_count);

#endif // GALLERY_MANAGER_H
