#ifndef FEATURES_H
#define FEATURES_H

#include QMK_KEYBOARD_H  // QMKの主要な型定義

// --- 設定定数 ---
#define GESTURE_MIN_ACCUMULATED_MOVEMENT 45
#define MOUSE_ACTION_COOLDOWN_MS 100
#define HORIZONTAL_SENSITIVITY_FACTOR 2
#define VERTICAL_SENSITIVITY_FACTOR 2
#define CLICKABLE_RESET_TIME 800
#define CLICKABLE_MIN_MOVEMENT 4

// --- Tap Dance ---
enum tap_dance_keycodes {
    TD_EN_LGUI_LANG = 0,
};

// --- カスタムキーマップコード ---
enum custom_keycodes {
    BS_MO5 = 0x9F00,
    EN_LGUI,
    GESTURE,
    ENT_MO4,
    JP_MO2,
    L_MO3,
};

// --- レイヤー定義 ---
enum custom_layers {
    LAYER_0 = 0,
    LAYER_1,
    LAYER_2,
    LAYER_3,
    LAYER_4,
    LAYER_5,
};

// --- マウスジェスチャー状態 ---
typedef enum {
    MOUSE_ACTION_STATE_NONE = 0,
    MOUSE_ACTION_STATE_FOR_LEFT_SWIPE,
    MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE,
    MOUSE_ACTION_STATE_FOR_UP_SWIPE,
    MOUSE_ACTION_STATE_FOR_DOWN_SWIPE
} mouse_action_active_state_t;

// --- クリック状態 ---
typedef enum {
    NONE = 0,
    WAITING,
    CLICKABLE,
    CLICKING,
} click_state_t;

// --- ホールドアクション種別 ---
typedef enum {
    HOLD_TYPE_LAYER,
    HOLD_TYPE_KEYCODE,
} hold_action_type_t;

// --- タップ・ホールド状態 ---
typedef struct {
    bool key_pressed;
    bool active_for_hold;
    uint16_t timer;
    bool tap_action_done;
} tap_hold_state_t;

// --- タップ・ホールドキー設定 ---
typedef struct {
    tap_hold_state_t state;
    uint16_t tap_keycode;
    uint16_t hold_target;
    hold_action_type_t hold_type;
} tap_hold_key_config_t;

// --- タップ・ホールドキー識別子（tap_hold.c で定義する） ---
typedef enum {
    TH_BS_MO5,
    TH_JP_MO1,
    TH_L_MO3,
    TH_ENT_MO4,
    TH_EN_LGUI,
    TH_G_KEY,
    TH_KC_LALT,
    TH_KC_LCTRL,
    NUM_TAP_HOLD_KEYS,
} tap_hold_key_id_t;

#endif
