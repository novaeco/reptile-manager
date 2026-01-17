#include "gallery_manager.h"
#include "esp_log.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


static const char *TAG = "GALLERY_MGR";
static const char *GALLERY_PATH = "/sdcard/imgs";

bool gallery_is_available(void) {
  struct stat st;
  if (stat(GALLERY_PATH, &st) == 0) {
    return S_ISDIR(st.st_mode);
  }
  return false;
}

int gallery_get_items(gallery_item_t *items, int max_count) {
  DIR *dir = opendir(GALLERY_PATH);
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open gallery dir: %s", GALLERY_PATH);
    return 0;
  }

  struct dirent *entry;
  int count = 0;
  while ((entry = readdir(dir)) != NULL && count < max_count) {
    if (entry->d_type == DT_REG) {
      // Check extension (optional, basic check)
      // Just take all files for now

      snprintf(items[count].full_path, MAX_GALLERY_PATH, "%s/%s", GALLERY_PATH,
               entry->d_name);
      strncpy(items[count].display_name, entry->d_name, MAX_GALLERY_NAME - 1);
      items[count].display_name[MAX_GALLERY_NAME - 1] = '\0';

      count++;
    }
  }
  closedir(dir);
  return count;
}
