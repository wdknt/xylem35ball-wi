//#include QMK_KEYBOARD_H
//const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {};

#include QMK_KEYBOARD_H

enum custom_keycodes {
    SCRL_MOD = SAFE_RANGE,
};
static bool scroll_mode;

static inline int16_t scale_scroll(int16_t v) {
    long tmp = (long)v * SCROLL_SCALE_NUM;
    tmp /= SCROLL_SCALE_DEN;
    if (tmp > 127) tmp = 127;
    if (tmp < -127) tmp = -127;
    return (int16_t)tmp;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == SCRL_MOD) {
        scroll_mode = record->event.pressed;
        return false;
    }
    return true;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    if (!scroll_mode) return mouse_report;

    int16_t dx = mouse_report.x;
    int16_t dy = mouse_report.y;
    mouse_report.x = mouse_report.y = 0;

//    int16_t v = scale_scroll(-dy);
//    int16_t h = scale_scroll(dx);

//    int16_t nh = mouse_report.h + h;
//    int16_t nv = mouse_report.v + v;

    // スクロール値を即時反映（新コード）
    mouse_report.h = scale_scroll(dx);
    mouse_report.v = scale_scroll(-dy);

    nh = nh > 127 ? 127 : (nh < -127 ? -127 : nh);
    nv = nv > 127 ? 127 : (nv < -127 ? -127 : nv);

    mouse_report.h = (int8_t)nh;
    mouse_report.v = (int8_t)nv;
    return mouse_report;
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        /* 1行目（10キー）*/ KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P,
        /* 2行目 */           KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_ENT,
        /* 3行目 */           KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_NO, KC_NO, KC_NO,
        /* 4行目（下の横並び5キー）*/ SCRL_MOD, KC_NO, KC_NO, KC_NO, KC_NO
    )
};
