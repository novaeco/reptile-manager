/**
 * @file ui_theme.h
 * @brief UI Color Palette and Theme Styles
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

// ====================================================================================
// COLOR THEME
// ====================================================================================

// Main theme - Premium Jungle/Terrarium inspired
// Enhanced color palette for better visual impact

// Backgrounds - Deeper, richer colors
#define COLOR_BG_DARK lv_color_hex(0x0A1510) // Deep jungle black
#define COLOR_BG_CARD                                                          \
  lv_color_hex(0x162B1D) // Dark terrarium green (glassmorphism base)
#define COLOR_BG_CARD_HOVER lv_color_hex(0x1E3A27) // Hover state
#define COLOR_ACCENT lv_color_hex(0x2D5A3D)        // Forest accent (richer)

// Primary colors - More vibrant
#define COLOR_PRIMARY lv_color_hex(0x00E676) // Neon green (eye-catching)
#define COLOR_PRIMARY_DARK                                                     \
  lv_color_hex(0x00C853) // Darker primary for pressed states
#define COLOR_SECONDARY lv_color_hex(0x69F0AE) // Mint green (secondary actions)

// Status colors - Higher contrast
#define COLOR_SUCCESS lv_color_hex(0x00E676) // Bright green (fed, healthy)
#define COLOR_WARNING lv_color_hex(0xFFAB00) // Amber (attention needed)
#define COLOR_DANGER lv_color_hex(0xFF5252)  // Bright red (urgent/overdue)
#define COLOR_INFO lv_color_hex(0x40C4FF)    // Cyan blue (informational)

// Text colors - Better readability
#define COLOR_TEXT lv_color_hex(0xF1F8E9)     // Almost white with green tint
#define COLOR_TEXT_DIM lv_color_hex(0xA5D6A7) // Muted green (secondary text)
#define COLOR_TEXT_MUTED                                                       \
  lv_color_hex(0x6B8E6B) // Even more muted (hints, disabled)

// UI elements
#define COLOR_BORDER lv_color_hex(0x43A047)          // Vibrant green border
#define COLOR_HEADER lv_color_hex(0x1B5E20)          // Dark green header
#define COLOR_HEADER_GRADIENT lv_color_hex(0x2E7D32) // Gradient end for header
#define COLOR_DIVIDER lv_color_hex(0x2E4A3A)         // Subtle divider lines

// Reptile-specific colors - More distinctive
#define COLOR_SNAKE lv_color_hex(0xA1887F)     // Warm brown for snakes
#define COLOR_LIZARD lv_color_hex(0x81C784)    // Fresh green for lizards
#define COLOR_TURTLE lv_color_hex(0x8D6E63)    // Earth brown for turtles
#define COLOR_EGG lv_color_hex(0xFFF8E1)       // Cream white for eggs
#define COLOR_AMPHIBIAN lv_color_hex(0x4DD0E1) // Cyan for amphibians

// Interactive states
#define COLOR_PRESSED lv_color_hex(0x00C853)  // Pressed button state
#define COLOR_DISABLED lv_color_hex(0x37474F) // Disabled elements

#endif // UI_THEME_H
