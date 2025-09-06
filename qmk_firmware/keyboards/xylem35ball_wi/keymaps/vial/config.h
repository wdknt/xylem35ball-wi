#pragma once
#define VIAL_KEYBOARD_UID {0xcd, 0x28, 0xc1, 0xff, 0xee, 0xf0, 0xe7, 0x19}
#define VIAL_TAP_DANCE_ENTRIES 8
#define VIAL_COMBO_ENTRIES 8
#define PICO_FLASH_SIZE_BYTES (1 * 1024 * 1024)
#define POINTING_DEVICE_INVERT_X
#define POINTING_DEVICE_INVERT_Y

// 既存の #define の下に追加
#define PMW3360_CPI 1200        // トラックボールの解像度
#define SCROLL_SCALE_NUM 3
#define SCROLL_SCALE_DEN 8
