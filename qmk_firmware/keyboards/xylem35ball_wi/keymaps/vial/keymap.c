#include QMK_KEYBOARD_H

// --- スクロール関連設定 ----------------------------------------------------
static bool scroll_mode = false;

// 速度プリセット（{分子, 分母} = 倍率）
// 例: {1,4}=0.25x, {1,2}=0.5x, {1,1}=1x, {2,1}=2x, {4,1}=4x
static const int8_t scroll_presets[][2] = {
    {1, 4},
    {1, 2},
    {1, 1},
    {2, 1},
    {4, 1},
};
#define NUM_SCROLL_PRESETS (sizeof(scroll_presets) / sizeof(scroll_presets[0]))
static uint8_t current_preset = 2;  // デフォルト: 1x

static inline int16_t scale_scroll(int16_t v) {
    // 現在のプリセットを適用
    long tmp = (long)v * scroll_presets[current_preset][0];
    tmp     /=        scroll_presets[current_preset][1];
    if (tmp > 127)  tmp = 127;
    if (tmp < -127) tmp = -127;
    return (int16_t)tmp;
}

// --- カスタムキー -----------------------------------------------------------
enum custom_keycodes {
    SCRL_MOD = SAFE_RANGE,  // 押している間だけスクロールモード
    SCRL_NEXT,              // 次の速度プリセットへ
    SCRL_PREV,              // 前の速度プリセットへ
    SCRL_PRINT              // 現在の速度をテキストとして出力
};

// レイヤー名（0〜4）
enum {
    _L0 = 0,
    _L1,
    _L2,
    _L3,
    _L4,
};

// --- キー入力処理 -----------------------------------------------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case SCRL_MOD:
                scroll_mode = true;      // 押している間だけスクロールモード
                return false;
            case SCRL_NEXT:              // 次の速度へ切り替え
                current_preset = (current_preset + 1) % NUM_SCROLL_PRESETS;
                return false;
            case SCRL_PREV:              // 前の速度へ切り替え
                current_preset = (current_preset + NUM_SCROLL_PRESETS - 1) % NUM_SCROLL_PRESETS;
                return false;
            case SCRL_PRINT: {
                // 現在のプリセットを文字列にして出力
                char buffer[32];
                int num = scroll_presets[current_preset][0];
                int den = scroll_presets[current_preset][1];

                // 例: "Scroll Speed: 1/2"
                snprintf(buffer, sizeof(buffer), "Scroll Speed: %d/%d\n", num, den);

                // キーボード入力として出力（メモ帳などで確認可能）
                send_string(buffer);
                return false;
            }
        }
    } else {
        if (keycode == SCRL_MOD) {
            scroll_mode = false;
            return false;
        }
    }
    return true;
}

// --- ポインティングデバイス処理 -------------------------------------------
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    if (!scroll_mode) {
        return mouse_report;  // 通常のマウス移動
    }

    // 現在の移動量をスクロールに変換（即時反映）
    int16_t dx = mouse_report.x;
    int16_t dy = mouse_report.y;

    mouse_report.x = 0;
    mouse_report.y = 0;
    mouse_report.h = scale_scroll(dx);   // 水平スクロール
    mouse_report.v = scale_scroll(-dy);  // 垂直スクロール（符号は逆向き）

    return mouse_report;
}

// --- キーマップ -------------------------------------------------------------
// SCRL_MOD, SCRL_PREV, SCRL_NEXT, SCRL_PRINT, KC_NO
// info.json の 10 + 10 + 10 + 5 物理配列に対応
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // ===== Layer 0 =====
    [_L0] = LAYOUT(
        // 1段目(10)
        TD(1),   KC_W,    KC_E,          KC_R,         KC_T,   KC_Y,   KC_U,   KC_I,     KC_O,            KC_P,
        // 2段目(10)
        KC_A,    KC_S,    KC_D,          LT(2, KC_F),  KC_G,   KC_H,   KC_J,   KC_K,     TD(4),           TD(0),
        // 3段目(10)
        LGUI_T(KC_Z), LSFT_T(KC_X), KC_C, KC_V,        KC_B,   KC_N,   KC_M,   KC_COMM,  LSFT_T(KC_DOT),  LGUI_T(KC_MINS),
        // 4段目(5)
        LT(1, KC_NO), LALT_T(KC_INT5), LCTL_T(KC_GRV), LT(3, KC_SPC), LT(2, KC_BSPC)
    ),

    // ===== Layer 1 =====
    [_L1] = LAYOUT(
        // 1段目(10)
        TD(1),        KC_F2,      KC_F3,      KC_F4,      KC_F5,      LCTL(KC_Z), LCTL(KC_X), LCTL(KC_C), LCTL(KC_V), QK_MACRO_0,
        // 2段目(10)
        KC_F6,        KC_F7,      KC_F8,      KC_F9,      KC_F10,     KC_ESC,     KC_BTN1,    KC_BTN2,    0x7e40,     TD(3),
        // 3段目(10)
        KC_LGUI,      KC_LSFT,    KC_C,       KC_V,       KC_B,       TD(2),      KC_NO,      KC_NO,      KC_LSFT,    KC_DEL,
        // 4段目(5)
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    // ===== Layer 2 =====
    [_L2] = LAYOUT(
        // 1段目(10)
        LSFT(KC_EQL), KC_LBRC,    LSFT(KC_3), LSFT(KC_4), LSFT(KC_5), KC_EQL,     LSFT(KC_6), LSFT(KC_LBRC), KC_INT3,    LSFT(KC_INT3),
        // 2段目(10)
        LSFT(KC_1),   LSFT(KC_RBRC), LSFT(KC_NUHS), LSFT(KC_8), LSFT(KC_9), KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, TD(0),
        // 3段目(10)
        LSFT(KC_SLSH), LSFT_T(KC_RBRC), KC_NUHS, KC_NO, KC_NO, KC_HOME, KC_PGDN, KC_PGUP, KC_END, KC_DEL,
        // 4段目(5)
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    // ===== Layer 3 =====
    [_L3] = LAYOUT(
        // 1段目(10)
        KC_1,   KC_2,   KC_3,        KC_4,        KC_5,   KC_6,   KC_7,   KC_8,   KC_9,   KC_0,
        // 2段目(10)
        LSFT(KC_7), KC_QUOT, LSFT(KC_MINS), KC_MINS, KC_SLSH, KC_BSPC, KC_4, KC_5, KC_6, TD(0),
        // 3段目(10)
        LSFT(KC_2), KC_SCLN, LSFT(KC_INT1), LSFT(KC_SCLN), LSFT(KC_QUOT), KC_0, KC_1, KC_2, KC_3, KC_DOT,
        // 4段目(5)
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_DEL
    ),
};
