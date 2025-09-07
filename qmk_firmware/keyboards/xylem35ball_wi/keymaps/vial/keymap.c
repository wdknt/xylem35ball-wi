//#include QMK_KEYBOARD_H
//const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {};

#include QMK_KEYBOARD_H

// カスタムキーコード
enum custom_keycodes {
    SCRL_MOD = SAFE_RANGE,  // 押している間だけスクロールモード
};

static bool scroll_mode = false;

// スクロール強度のスケーリング関数
static inline int16_t scale_scroll(int16_t v) {
    long tmp = (long)v * SCROLL_SCALE_NUM;
    tmp /= SCROLL_SCALE_DEN;
    if (tmp > 127)  tmp = 127;
    if (tmp < -127) tmp = -127;
    return (int16_t)tmp;
}

// ボタン処理
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case SCRL_MOD:
            scroll_mode = record->event.pressed;  // 押している間だけ有効
            return false;
    }
    return true;
}

// トラックボール処理
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    if (!scroll_mode) {
        return mouse_report; // そのまま返す
    }

    int16_t dx = mouse_report.x;
    int16_t dy = mouse_report.y;

    mouse_report.x = 0;
    mouse_report.y = 0;

    int16_t v = scale_scroll(-dy);
    int16_t h = scale_scroll( dx);

    int16_t nh = mouse_report.h + h;
    int16_t nv = mouse_report.v + v;

    if (nh > 127)  nh = 127;
    if (nh < -127) nh = -127;
    if (nv > 127)  nv = 127;
    if (nv < -127) nv = -127;

    mouse_report.h = (int8_t)nh;
    mouse_report.v = (int8_t)nv;

    return mouse_report;  // 最後に返す
}


// ===== キーマップ例 =====
// LAYOUT_xxx はあなたの keyboard.c に定義されたマクロに合わせてください。
// とりあえず 1 キーだけ定義して、SCRL_MOD を割り当てた例です。
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        SCRL_MOD  // ← ボタンを押している間だけスクロールモード
    )
};
