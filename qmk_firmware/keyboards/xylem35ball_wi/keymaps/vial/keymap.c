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
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        /* 1行目（10キー）*/ KC_Q,   KC_W,   KC_E,   KC_R,   KC_T,   KC_Y,   KC_U,   KC_I,   KC_O,   KC_P,
        /* 2行目（10キー）*/ KC_A,   KC_S,   KC_D,   KC_F,   KC_G,   KC_H,   KC_J,   KC_K,   KC_L,   KC_ENT,
        /* 3行目（10キー）*/ KC_Z,   KC_X,   KC_C,   KC_V,   KC_B,   KC_N,   KC_M,   KC_NO,  KC_NO,  KC_NO,
        /* 4行目（5キー）  */ SCRL_MOD, SCRL_PREV, SCRL_NEXT, SCRL_PRINT, KC_NO
    )
};
