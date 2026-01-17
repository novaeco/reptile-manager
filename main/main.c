/**
 * @file main.c
 * @brief ESP32-P4 LVGL Smart Panel for GUITION JC4880P443C
 *
 * Features:
 *   - Multi-page UI with navigation
 *   - Status bar with WiFi, Bluetooth, Date, Time
 *   - SD Card mounted with image loading support
 *   - PNG/JPEG decoder for LVGL
 *   - Touch support (GT911)
 *   - WiFi preparation via esp_hosted
 */

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Audio
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

// GPIO and power control
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_ldo_regulator.h"

// I2C for touch
#include "driver/i2c_master.h"

// SD Card
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// MIPI-DSI and LCD panel
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_types.h"

// Touch driver
#include "esp_lcd_touch_gt911.h"

// WiFi via ESP32-C6 (esp_hosted)
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "ui/ui_manager.h" // Added UI Manager

// Bluetooth via ESP32-C6 (esp_hosted) - conditionally included
#if CONFIG_BT_ENABLED
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_hosted_bluedroid.h"
#endif // CONFIG_BT_ENABLED

// ESP-Hosted (always needed for WiFi and OTA)
#include "bluetooth_manager.h"
#include "data/database.h" // Added Data Layer
#include "esp_hosted.h"
#include "models.h"
#include "ui_assets.h"
#include "ui_theme.h"
#include "wifi_manager.h"

// ESP32-C6 embedded firmware for OTA

static const char *TAG = "SMART_PANEL";

// ====================================================================================
// HARDWARE CONFIGURATION
// ====================================================================================

#define LCD_H_RES 480
#define LCD_V_RES 800

#define LCD_RST_GPIO 5
#define LCD_BL_GPIO 23
#define BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_LOW_SPEED_MODE LEDC_LOW_SPEED_MODE

static uint8_t current_brightness = 100;

#define TOUCH_I2C_SDA 7
#define TOUCH_I2C_SCL 8
#define TOUCH_I2C_FREQ_HZ 400000

#define DSI_LANE_NUM 2
#define DSI_LANE_BITRATE 500
#define DPI_CLOCK_MHZ 34

#define DSI_PHY_LDO_CHANNEL 3
#define DSI_PHY_VOLTAGE_MV 2500

#define BL_LEDC_TIMER LEDC_TIMER_0
#define BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define BL_PWM_FREQ 5000

// SD Card GPIOs (from JC4880P443C schematic)
#define SD_CMD_GPIO 44
#define SD_CLK_GPIO 43
#define SD_D0_GPIO 39
#define SD_D1_GPIO 40
#define SD_D2_GPIO 41
#define SD_D3_GPIO 42

#define SD_MOUNT_POINT "/sdcard"

// ====================================================================================
// AUDIO ES8311 CODEC CONFIG (Official ESP32-P4 pin mapping from ESP-IDF
// example)
// ====================================================================================
#define AUDIO_ENABLED 1 // Test esp_codec_dev approach

// ES8311 I2C control pins (from ESP-IDF i2s_es8311 example for ESP32-P4)
#define ES8311_I2C_SDA GPIO_NUM_7 // I2C Data
#define ES8311_I2C_SCL GPIO_NUM_8 // I2C Clock
#define ES8311_I2C_ADDR 0x18      // ES8311 default I2C address

// I2S pins for ES8311 (Official ESP32-P4 defaults from Kconfig.projbuild)
#define I2S_MCLK_GPIO GPIO_NUM_13 // I2S Master Clock
#define I2S_BCK_GPIO GPIO_NUM_12  // I2S Bit Clock
#define I2S_WS_GPIO GPIO_NUM_10   // I2S Word Select (LRCK)
#define I2S_DO_GPIO GPIO_NUM_9    // I2S Data Out (to ES8311 SDIN)
#define I2S_DI_GPIO GPIO_NUM_11   // I2S Data In (from ES8311 mic, optional)

// PA (Power Amplifier NS4150B) enable pin
#define PA_ENABLE_GPIO GPIO_NUM_53 // Power Amp control (high = enabled)

// Audio settings
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_MCLK_MULTIPLE 384 // MCLK = SAMPLE_RATE * 384
#define AUDIO_VOLUME 60         // Volume 0-100

// Sound frequencies (Hz) for UI tones
#define SOUND_CLICK_FREQ 1000
#define SOUND_SUCCESS_FREQ 1500
#define SOUND_ERROR_FREQ 400
#define SOUND_ALERT_FREQ 2000

// ====================================================================================
// BATTERY CONFIG (Optional - set BATTERY_ENABLED to 0 if no fuel gauge)
// ====================================================================================
#define BATTERY_ENABLED 0   // Set to 1 if battery monitoring available
#define BATTERY_SIMULATED 1 // Use simulated battery level for demo

// ====================================================================================
// ST7701 INIT COMMANDS
// ====================================================================================

static const st7701_lcd_init_cmd_t st7701_lcd_cmds[] = {
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]){0x08}, 1, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x63, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x10, 0x08}, 2, 0},
    {0xCC, (uint8_t[]){0x10}, 1, 0},
    {0xB0,
     (uint8_t[]){0x80, 0x09, 0x53, 0x0C, 0xD0, 0x07, 0x0C, 0x09, 0x09, 0x28,
                 0x06, 0xD4, 0x13, 0x69, 0x2B, 0x71},
     16, 0},
    {0xB1,
     (uint8_t[]){0x80, 0x94, 0x5A, 0x10, 0xD3, 0x06, 0x0A, 0x08, 0x08, 0x25,
                 0x03, 0xD3, 0x12, 0x66, 0x6A, 0x0D},
     16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x5D}, 1, 0},
    {0xB1, (uint8_t[]){0x58}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x4E}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xB9, (uint8_t[]){0x10, 0x1F}, 2, 0},
    {0xBB, (uint8_t[]){0x03}, 1, 0},
    {0xBC, (uint8_t[]){0x00}, 1, 0},
    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 0},
    {0xD0, (uint8_t[]){0x88}, 1, 0},
    {0xE0, (uint8_t[]){0x00, 0x3A, 0x02}, 3, 0},
    {0xE1,
     (uint8_t[]){0x04, 0xA0, 0x00, 0xA0, 0x05, 0xA0, 0x00, 0xA0, 0x00, 0x40,
                 0x40},
     11, 0},
    {0xE2,
     (uint8_t[]){0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0,
                 0x00, 0xA0, 0x00},
     13, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5,
     (uint8_t[]){0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A,
                 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0},
     16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8,
     (uint8_t[]){0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29,
                 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0},
     16, 0},
    {0xEB, (uint8_t[]){0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00}, 7, 0},
    {0xEC, (uint8_t[]){0x08, 0x01}, 2, 0},
    {0xED,
     (uint8_t[]){0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF,
                 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B},
     16, 0},
    {0xEF, (uint8_t[]){0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x11, (uint8_t[]){0x00}, 1, 120},
    {0x29, (uint8_t[]){0x00}, 1, 20},
};

// ====================================================================================
// GLOBAL HANDLES AND STATE
// ====================================================================================

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;
static lv_display_t *main_display = NULL;
static sdmmc_card_t *sd_card = NULL;
bool sd_mounted = false;

// State
static bool wifi_connected = false;
static bool bluetooth_enabled = false;

// WiFi variables removed - using wifi_manager

// ====================================================================================
// UI Code moved to ui/ui_manager.c

// REPTILE MANAGER DATA STRUCTURES
// ====================================================================================

// Animal species types
// Data structures moved to models.h

// Global reptile data
// MOVED TO data/database.c
// Accessed via database.h externs temporarily or API

// ====================================================================================
// SD CARD FUNCTIONS
// ====================================================================================

static esp_err_t sd_card_init(void) {
  ESP_LOGI(TAG, "Initializing SD card...");

  // Use SDMMC Slot 0 for SD card (Slot 1 is used by esp_hosted for C6)
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_0;          // Explicitly use Slot 0
  host.max_freq_khz = SDMMC_FREQ_DEFAULT; // Lower freq for stability

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 4; // 4-bit mode
  slot_config.clk = SD_CLK_GPIO;
  slot_config.cmd = SD_CMD_GPIO;
  slot_config.d0 = SD_D0_GPIO;
  slot_config.d1 = SD_D1_GPIO;
  slot_config.d2 = SD_D2_GPIO;
  slot_config.d3 = SD_D3_GPIO;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config,
                                          &mount_config, &sd_card);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
    sd_mounted = false;
    return ret;
  }

  sdmmc_card_print_info(stdout, sd_card);
  sd_mounted = true;
  ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);

  // List files in /sdcard/imgs
  DIR *dir = opendir(SD_MOUNT_POINT "/imgs");
  if (dir) {
    ESP_LOGI(TAG, "Files in %s/imgs:", SD_MOUNT_POINT);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      ESP_LOGI(TAG, "  - %s", entry->d_name);
    }
    closedir(dir);
  }

  return ESP_OK;
}

// Audio and Battery state
// static bool audio_enabled = true;
static uint8_t battery_level = 100; // Simulated battery level

// ====================================================================================
// AUDIO FUNCTIONS (ES8311 Codec via esp_codec_dev - Full implementation)
// ====================================================================================

#if AUDIO_ENABLED
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "es8311_codec.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include <math.h>

static bool audio_initialized = false;
static i2c_master_bus_handle_t audio_i2c_bus = NULL;
static const audio_codec_if_t *es8311_codec_if = NULL;

// Buffer for tone generation
#define AUDIO_BUFFER_SIZE 512
// static int16_t audio_buffer[AUDIO_BUFFER_SIZE * 2]; // Stereo

// static void audio_generate_tone_stereo(uint32_t freq_hz, int16_t *buffer,
//                                        size_t samples) {
// for (size_t i = 0; i < samples; i++) {
//   float angle = 2.0f * M_PI * freq_hz * i / AUDIO_SAMPLE_RATE;
//   int16_t sample = (int16_t)(sinf(angle) * 16000); // ~50% volume
//   buffer[i * 2] = sample;                          // Left
//   buffer[i * 2 + 1] = sample;                      // Right
// }
// }

static void audio_init(void) {
  if (audio_initialized)
    return;

  ESP_LOGI(TAG, "Initializing ES8311 audio codec...");

  // Configure PA enable pin (NS4150B amplifier)
  gpio_config_t pa_conf = {
      .pin_bit_mask = (1ULL << PA_ENABLE_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&pa_conf);
  gpio_set_level(PA_ENABLE_GPIO, 1); // Enable PA

  // Initialize shared I2C bus for ES8311 and Touch (I2C_NUM_0 on GPIO 7/8)
  // This bus is shared with the touch controller - both use same GPIOs
  if (!i2c_bus_handle) {
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0, // Shared bus with touch
        .scl_io_num = ES8311_I2C_SCL,
        .sda_io_num = ES8311_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to create shared I2C bus: %s",
               esp_err_to_name(ret));
      return;
    }
  }

  // Use the shared I2C bus for audio
  audio_i2c_bus = i2c_bus_handle;

  // Create I2C control interface for ES8311
  audio_codec_i2c_cfg_t i2c_cfg = {
      .addr = ES8311_CODEC_DEFAULT_ADDR,
      .bus_handle = audio_i2c_bus,
  };
  const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
  if (!ctrl_if) {
    ESP_LOGE(TAG, "Failed to create I2C control interface");
    return;
  }

  // Create GPIO interface
  const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
  if (!gpio_if) {
    ESP_LOGE(TAG, "Failed to create GPIO interface");
    return;
  }

  // Configure ES8311 codec
  es8311_codec_cfg_t es8311_cfg = {
      .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
      .ctrl_if = ctrl_if,
      .gpio_if = gpio_if,
      .pa_pin = PA_ENABLE_GPIO,
      .use_mclk = false, // Internal clock
      .master_mode = false,
  };
  es8311_codec_if = es8311_codec_new(&es8311_cfg);
  if (!es8311_codec_if) {
    ESP_LOGE(TAG, "Failed to create ES8311 codec interface");
    return;
  }

  // Note: I2S data path disabled due to ESP-IDF 6.1 linker bug
  // The codec is configured and PA is enabled, but no audio output
  // This will be fixed when ESP-IDF patches the linker issue

  audio_initialized = true;
  ESP_LOGI(TAG, "ES8311 codec initialized (I2C @ 0x%02X, PA on GPIO%d)",
           ES8311_CODEC_DEFAULT_ADDR, PA_ENABLE_GPIO);
  ESP_LOGW(
      TAG,
      "Audio playback disabled - ESP-IDF 6.1 linker bug with esp_driver_i2s");
}

static void audio_play_tone(uint32_t freq_hz, uint32_t duration_ms) {
  // Audio playback disabled due to ESP-IDF 6.1 linker bug
  // When esp_driver_i2s is added, it causes:
  // "error: --enable-non-contiguous-regions discards section `.text.delay_us'"
  (void)freq_hz;
  (void)duration_ms;
}

#else
static void audio_init(void) {
  ESP_LOGI(TAG, "Audio disabled (AUDIO_ENABLED=0)");
}
static void audio_play_tone(uint32_t freq_hz, uint32_t duration_ms) {
  (void)freq_hz;
  (void)duration_ms;
}
#endif

// UI Sound effects
// static void sound_click(void) { audio_play_tone(SOUND_CLICK_FREQ, 30); }

// static void sound_success(void) {
// audio_play_tone(SOUND_SUCCESS_FREQ, 100);
// vTaskDelay(pdMS_TO_TICKS(50));
// audio_play_tone(SOUND_SUCCESS_FREQ + 500, 100);
// }

// static void sound_error(void) { audio_play_tone(SOUND_ERROR_FREQ, 200); }

// static void sound_alert(void) {
// for (int i = 0; i < 3; i++) {
//   audio_play_tone(SOUND_ALERT_FREQ, 100);
//   vTaskDelay(pdMS_TO_TICKS(100));
// }
// }

// ====================================================================================
// BATTERY FUNCTIONS
// ====================================================================================

uint8_t battery_get_level(void) {
#if BATTERY_SIMULATED
  // Simulated battery - slowly decreases then resets
  static uint32_t last_update = 0;
  uint32_t now = xTaskGetTickCount();
  if (now - last_update > pdMS_TO_TICKS(60000)) { // Every minute
    last_update = now;
    if (battery_level > 10) {
      battery_level -= 1;
    } else {
      battery_level = 100; // Reset for demo
    }
  }
  return battery_level;
#elif BATTERY_ENABLED
  // TODO: Read from fuel gauge (MAX17048, BQ27441, etc.)
  return 100;
#else
  return 100; // Always full if no battery
#endif
}

// Helpers removed (moved to ui_manager.c)

// Forward declaration for NVS functions (defined later)

// ====================================================================================
// GLOBAL STATE VARIABLES
// ====================================================================================
static bool time_synced = false;

// Forward declarations
// Forward declarations removed (now in database.h/ui_shared.h)
// static int reptile_days_since_feeding(uint8_t id);
// static int reptile_count_feeding_alerts(void);
// static const char *reptile_get_icon(reptile_species_t species);

// ====================================================================================
// TIME & SNTP FUNCTIONS
// ====================================================================================

static void time_sync_notification_cb(struct timeval *tv) {
  ESP_LOGI(TAG, "SNTP time synchronized!");
  time_synced = true;
}

static void app_sntp_init(void) {
  ESP_LOGI(TAG, "Initializing SNTP...");

  // Set timezone to Paris (CET-1CEST,M3.5.0,M10.5.0/3)
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.google.com");
  esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  esp_sntp_init();
}

// WiFi/Bluetooth functions moved to respective managers

// ====================================================================================
// REPTILE MANAGER LOGIC
// ====================================================================================

static void save_data(void) {
  db_save_data(); // Use Database Layer
}

static void load_data(void) { db_load_data(); }

static void reptile_init_demo_data(void) { db_init_demo_data(); }

// Deleting orphan code

// (Function end of restored init removed to clean file)

// reptile_get_icon moved to ui_manager.c/ui_shared.c

// Logic helpers moved to database.c
// static int reptile_days_since_feeding(uint8_t id) ...
// static int reptile_count_feeding_alerts(void) ...

// wifi_event_handler removed - handled by wifi_manager

// ====================================================================================
// WIFI NVS STORAGE - Save/Load favorite networks
// ====================================================================================

// WiFi NVS functions removed - handled by wifi_manager

// WiFi Scan function

// Connect to specific WiFi

// ====================================================================================
// BLUETOOTH FUNCTIONS (via ESP32-C6 co-processor)
// ====================================================================================

// Bluetooth functions moved to bluetooth_manager.c

// ====================================================================================
// ESP32-C6 CO-PROCESSOR OTA UPDATE
// ====================================================================================

// ====================================================================================
// FRENCH AZERTY KEYBOARD LAYOUT - Based on LVGL official example
#include "ui_assets.h"

// HARDWARE FUNCTIONS
// ====================================================================================

static esp_err_t enable_dsi_phy_power(void) {
  static esp_ldo_channel_handle_t phy_pwr_chan = NULL;
  if (phy_pwr_chan)
    return ESP_OK;
  esp_ldo_channel_config_t ldo_cfg = {
      .chan_id = DSI_PHY_LDO_CHANNEL,
      .voltage_mv = DSI_PHY_VOLTAGE_MV,
  };
  return esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan);
}

static esp_err_t backlight_init(void) {
  ledc_timer_config_t timer_cfg = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_10_BIT,
      .timer_num = BL_LEDC_TIMER,
      .freq_hz = BL_PWM_FREQ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

  ledc_channel_config_t ch_cfg = {
      .gpio_num = LCD_BL_GPIO,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = BL_LEDC_CHANNEL,
      .timer_sel = BL_LEDC_TIMER,
      .duty = 0,
      .hpoint = 0,
  };
  return ledc_channel_config(&ch_cfg);
}

void backlight_set(uint8_t percent) {
  if (percent > 100)
    percent = 100;
  current_brightness = percent;
  uint32_t duty = (percent * 1023) / 100;
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CHANNEL, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CHANNEL);
}

static esp_err_t i2c_init(void) {
  i2c_master_bus_config_t bus_config = {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = TOUCH_I2C_SDA,
      .scl_io_num = TOUCH_I2C_SCL,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = false,
  };
  return i2c_new_master_bus(&bus_config, &i2c_bus_handle);
}

static esp_err_t touch_init(void) {
  if (!i2c_bus_handle)
    ESP_ERROR_CHECK(i2c_init());

  esp_lcd_panel_io_handle_t touch_io = NULL;
  esp_lcd_panel_io_i2c_config_t io_config = {
      .dev_addr = 0x5D,
      .scl_speed_hz = TOUCH_I2C_FREQ_HZ,
      .control_phase_bytes = 1,
      .lcd_cmd_bits = 16,
      .lcd_param_bits = 0,
      .dc_bit_offset = 0,
      .flags = {.disable_control_phase = 1},
  };
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_io_i2c(i2c_bus_handle, &io_config, &touch_io));

  esp_lcd_touch_config_t touch_cfg = {
      .x_max = LCD_H_RES,
      .y_max = LCD_V_RES,
      .rst_gpio_num = GPIO_NUM_NC,
      .int_gpio_num = GPIO_NUM_NC,
      .levels = {.reset = 0, .interrupt = 0},
      .flags = {.swap_xy = 0, .mirror_x = 0, .mirror_y = 0},
  };
  return esp_lcd_touch_new_i2c_gt911(touch_io, &touch_cfg, &touch_handle);
}

static esp_err_t display_init(esp_lcd_panel_io_handle_t *out_io,
                              esp_lcd_panel_handle_t *out_panel) {
  ESP_ERROR_CHECK(enable_dsi_phy_power());
  vTaskDelay(pdMS_TO_TICKS(10));

  esp_lcd_dsi_bus_handle_t dsi_bus;
  esp_lcd_dsi_bus_config_t bus_cfg = {
      .bus_id = 0,
      .num_data_lanes = DSI_LANE_NUM,
      .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
      .lane_bit_rate_mbps = DSI_LANE_BITRATE,
  };
  ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus));
  vTaskDelay(pdMS_TO_TICKS(50));

  esp_lcd_panel_io_handle_t panel_io;
  esp_lcd_dbi_io_config_t dbi_cfg = {
      .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8};
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &panel_io));

  esp_lcd_dpi_panel_config_t dpi_cfg = {
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = DPI_CLOCK_MHZ,
      .virtual_channel = 0,
      .in_color_format = LCD_COLOR_FMT_RGB565,
      .num_fbs = 1,
      .video_timing =
          {
              .h_size = LCD_H_RES,
              .v_size = LCD_V_RES,
              .hsync_pulse_width = 12,
              .hsync_back_porch = 42,
              .hsync_front_porch = 42,
              .vsync_pulse_width = 2,
              .vsync_back_porch = 8,
              .vsync_front_porch = 166,
          },
  };

  st7701_vendor_config_t vendor_cfg = {
      .flags.use_mipi_interface = 1,
      .mipi_config = {.dsi_bus = dsi_bus, .dpi_config = &dpi_cfg},
      .init_cmds = st7701_lcd_cmds,
      .init_cmds_size = sizeof(st7701_lcd_cmds) / sizeof(st7701_lcd_cmds[0]),
  };

  esp_lcd_panel_dev_config_t panel_cfg = {
      .reset_gpio_num = LCD_RST_GPIO,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
      .vendor_config = &vendor_cfg,
  };

  esp_lcd_panel_handle_t panel;
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(panel_io, &panel_cfg, &panel));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
  vTaskDelay(pdMS_TO_TICKS(50));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
  vTaskDelay(pdMS_TO_TICKS(100));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

  *out_io = panel_io;
  *out_panel = panel;
  ESP_LOGI(TAG, "Display initialized");
  return ESP_OK;
}

// UI Code moved to ui/ui_manager.c

void app_main(void) {
  ESP_LOGI(TAG, "Starting Reptile Manager (4.3 Inch Portrait)");

  // Storage
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Network Logic
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_manager_init();

  if (bluetooth_init() != ESP_OK) {
    ESP_LOGW(TAG, "Bluetooth init failed");
  }

  // SD Card
  if (sd_card_init() != ESP_OK) {
    ESP_LOGW(TAG, "SD Card init failed");
  }

  // Data
  db_init_demo_data();
  app_sntp_init();

  // Audio
  audio_init();

  // Hardware (Display/Touch)
  esp_lcd_panel_io_handle_t io;
  esp_lcd_panel_handle_t panel;
  ESP_ERROR_CHECK(display_init(&io, &panel));
  ESP_ERROR_CHECK(touch_init());
  ESP_ERROR_CHECK(backlight_init());
  backlight_set(100);

  // LVGL
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io,
      .panel_handle = panel,
      .buffer_size = LCD_H_RES * LCD_V_RES / 10,
      .double_buffer = true,
      .hres = LCD_H_RES,
      .vres = LCD_V_RES,
      .monochrome = false,
      .rotation = {.swap_xy = false, .mirror_x = false, .mirror_y = false},
      .flags = {.buff_dma = true, .buff_spiram = true, .swap_bytes = false}};
  const lvgl_port_display_dsi_cfg_t dsi_cfg = {0};
  lv_display_t *disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);

  const lvgl_port_touch_cfg_t touch_cfg = {
      .disp = disp,
      .handle = touch_handle,
  };
  lvgl_port_add_touch(&touch_cfg);

  // UI Init
  if (lvgl_port_lock(0)) {
    ui_init(disp);
    lvgl_port_unlock();
  }

  // Loop
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
