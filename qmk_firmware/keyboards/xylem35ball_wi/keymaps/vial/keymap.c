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

// --- “滑らかスクロール”設定（好みに応じて調整可） --------------------------
// 固定小数点で微小量を積み上げ、EMAでガタつきを抑え、入力ゼロ時は慣性で減衰
#ifndef SCROLL_FP_SHIFT
#    define SCROLL_FP_SHIFT 8     // 固定小数点：小数ビット数（24.8）
#endif
#ifndef SCROLL_ALPHA_SHIFT
#    define SCROLL_ALPHA_SHIFT 2  // EMA係数 1/(2^n) → 2: 1/4（数値↑でより滑らか＝反応は緩やか）
#endif
#ifndef SCROLL_INERTIA_SHIFT
#    define SCROLL_INERTIA_SHIFT 3 // 入力0時の減衰 1/(2^n) → 3: 1/8（数値↓でキビキビ止まる）
#endif
#ifndef SCROLL_MAX_STEP
#    define SCROLL_MAX_STEP 6     // 1レポートで送る最大スクロール量（跳ね防止）
#endif

// 平滑化用の内部状態
static int32_t filt_x = 0, filt_y = 0;  // EMA内部値（生値ベース）
static int32_t acc_h  = 0, acc_v  = 0;  // 固定小数点の累積

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
// EMA＋慣性＋小数累積で“ぬるっ”と動かす
report_mouse_t pointing_device_task_user(report_mouse_t m) {
    if (!scroll_mode) {
        return m;  // 通常のマウス移動
    }

    // 入力取得＆カーソル移動は止める
    int32_t dx = m.x;
    int32_t dy = m.y;
    m.x = 0;
    m.y = 0;

    // ===== EMA（平滑化）=====
    // filt += (input - filt) * alpha ; alpha = 1/(2^SCROLL_ALPHA_SHIFT)
    filt_x += ((dx - filt_x) >> SCROLL_ALPHA_SHIFT);
    filt_y += ((dy - filt_y) >> SCROLL_ALPHA_SHIFT);

    // 入力が止まったら慣性でゆっくり0へ
    if (dx == 0 && dy == 0) {
        filt_x -= (filt_x >> SCROLL_INERTIA_SHIFT);
        filt_y -= (filt_y >> SCROLL_INERTIA_SHIFT);
    }

    // 平滑化後の値を整数化
    int16_t sdx = (int16_t)filt_x;
    int16_t sdy = (int16_t)filt_y;

    // プリセット倍率を適用（線形）
    int16_t h_raw = scale_scroll(sdx);
    int16_t v_raw = scale_scroll(-sdy);

    // ===== 固定小数点で微小量を蓄積 =====
    acc_h += ((int32_t)h_raw << SCROLL_FP_SHIFT);
    acc_v += ((int32_t)v_raw << SCROLL_FP_SHIFT);

    int16_t out_h = (int16_t)(acc_h >> SCROLL_FP_SHIFT);
    int16_t out_v = (int16_t)(acc_v >> SCROLL_FP_SHIFT);

    acc_h -= ((int32_t)out_h << SCROLL_FP_SHIFT);
    acc_v -= ((int32_t)out_v << SCROLL_FP_SHIFT);

    // 1レポートの最大量を制限（急な跳ね抑制）
    if (out_h >  SCROLL_MAX_STEP) out_h =  SCROLL_MAX_STEP;
    if (out_h < -SCROLL_MAX_STEP) out_h = -SCROLL_MAX_STEP;
    if (out_v >  SCROLL_MAX_STEP) out_v =  SCROLL_MAX_STEP;
    if (out_v < -SCROLL_MAX_STEP) out_v = -SCROLL_MAX_STEP;

    // HID範囲クランプ
    if (out_h > 127)  out_h = 127;
    if (out_h < -127) out_h = -127;
    if (out_v > 127)  out_v = 127;
    if (out_v < -127) out_v = -127;

    m.h = (int8_t)out_h;   // 水平スクロール
    m.v = (int8_t)out_v;   // 垂直スクロール
    return m;
}

// --- キーマップ -------------------------------------------------------------
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
