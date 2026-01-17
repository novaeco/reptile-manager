#include "ui_assets.h"

// ====================================================================================
// FRENCH AZERTY KEYBOARD LAYOUT - Based on LVGL official example
// ====================================================================================

// Lowercase AZERTY layout
const char *kb_map_azerty_lower[] = {"1",
                                     "2",
                                     "3",
                                     "4",
                                     "5",
                                     "6",
                                     "7",
                                     "8",
                                     "9",
                                     "0",
                                     LV_SYMBOL_BACKSPACE,
                                     "\n",
                                     "a",
                                     "z",
                                     "e",
                                     "r",
                                     "t",
                                     "y",
                                     "u",
                                     "i",
                                     "o",
                                     "p",
                                     "\n",
                                     "q",
                                     "s",
                                     "d",
                                     "f",
                                     "g",
                                     "h",
                                     "j",
                                     "k",
                                     "l",
                                     "m",
                                     LV_SYMBOL_NEW_LINE,
                                     "\n",
                                     "ABC",
                                     "w",
                                     "x",
                                     "c",
                                     "v",
                                     "b",
                                     "n",
                                     ",",
                                     ".",
                                     "-",
                                     "\n",
                                     "1#",
                                     LV_SYMBOL_LEFT,
                                     " ",
                                     " ",
                                     " ",
                                     LV_SYMBOL_RIGHT,
                                     LV_SYMBOL_OK,
                                     ""};

// Uppercase AZERTY layout
const char *kb_map_azerty_upper[] = {"!",
                                     "@",
                                     "#",
                                     "$",
                                     "%",
                                     "^",
                                     "&",
                                     "*",
                                     "(",
                                     ")",
                                     LV_SYMBOL_BACKSPACE,
                                     "\n",
                                     "A",
                                     "Z",
                                     "E",
                                     "R",
                                     "T",
                                     "Y",
                                     "U",
                                     "I",
                                     "O",
                                     "P",
                                     "\n",
                                     "Q",
                                     "S",
                                     "D",
                                     "F",
                                     "G",
                                     "H",
                                     "J",
                                     "K",
                                     "L",
                                     "M",
                                     LV_SYMBOL_NEW_LINE,
                                     "\n",
                                     "abc",
                                     "W",
                                     "X",
                                     "C",
                                     "V",
                                     "B",
                                     "N",
                                     ";",
                                     ":",
                                     "_",
                                     "\n",
                                     "1#",
                                     LV_SYMBOL_LEFT,
                                     " ",
                                     " ",
                                     " ",
                                     LV_SYMBOL_RIGHT,
                                     LV_SYMBOL_OK,
                                     ""};

// Special characters layout
const char *kb_map_special[] = {"1",
                                "2",
                                "3",
                                "4",
                                "5",
                                "6",
                                "7",
                                "8",
                                "9",
                                "0",
                                LV_SYMBOL_BACKSPACE,
                                "\n",
                                "+",
                                "-",
                                "*",
                                "/",
                                "=",
                                "_",
                                "<",
                                ">",
                                "[",
                                "]",
                                "\n",
                                "{",
                                "}",
                                "|",
                                "\\",
                                "~",
                                "`",
                                "'",
                                "\"",
                                ":",
                                ";",
                                LV_SYMBOL_NEW_LINE,
                                "\n",
                                "abc",
                                "@",
                                "#",
                                "$",
                                "%",
                                "^",
                                "&",
                                ",",
                                ".",
                                "?",
                                "\n",
                                "ABC",
                                LV_SYMBOL_LEFT,
                                " ",
                                " ",
                                " ",
                                LV_SYMBOL_RIGHT,
                                LV_SYMBOL_OK,
                                ""};

// Control map for keyboard buttons (defines button widths and special flags)
// LVGL 9 uses LV_KEYBOARD_CTRL_BTN_FLAGS for buttons that should change mode
// Row 1: 11 buttons (numbers + backspace)
// Row 2: 10 buttons (letters a-p)
// Row 3: 11 buttons (letters q-m + enter)
// Row 4: 10 buttons (shift + letters + punctuation)
// Row 5: 7 buttons (123 + arrows + spaces + OK)

const lv_buttonmatrix_ctrl_t kb_ctrl_lower[] = {
    // Row 1: numbers + backspace (wider)
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    // Row 2: letters
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    // Row 3: letters + enter (wider)
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    // Row 4: ABC (mode switch) + letters + punctuation
    6 | KB_CTRL_MODE_BTN, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    // Row 5: 123 (mode switch) + arrows + spaces + OK
    5 | KB_CTRL_MODE_BTN, 3, 7, 7, 7, 3, 5 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG};

const lv_buttonmatrix_ctrl_t kb_ctrl_upper[] = {
    // Row 1: numbers + backspace
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    // Row 2: letters
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    // Row 3: letters + enter
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    // Row 4: abc (mode switch) + letters
    6 | KB_CTRL_MODE_BTN, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    // Row 5: 123 + arrows + spaces + OK
    5 | KB_CTRL_MODE_BTN, 3, 7, 7, 7, 3, 5 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG};

const lv_buttonmatrix_ctrl_t kb_ctrl_special[] = {
    // Row 1: numbers + backspace
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    // Row 2: special chars
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    // Row 3: special chars + enter
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG,
    // Row 4: abc (mode switch) + special chars
    6 | KB_CTRL_MODE_BTN, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    // Row 5: ABC + arrows + spaces + OK
    5 | KB_CTRL_MODE_BTN, 3, 7, 7, 7, 3, 5 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG};
