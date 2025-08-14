#ifndef FEATURES_H
#define FEATURES_H

#include QMK_KEYBOARD_H  // QMKの主要な型定義

// --- Tap Dance ---
enum tap_dance_keycodes {
    TD_EN_LGUI_LANG = 0,
    TD_BTN2_LALT_LANG,
};

// --- カスタムキーマップコード ---
enum custom_keycodes {
    BS_MO1 = 0x9F00,
    EN_LGUI,
    ENT_MO5,
    JP_MO1,
};

// --- タップ・ホールドキー識別子（tap_hold.c で定義する） ---
typedef enum {
    TH_BS_MO5,
    TH_JP_MO1,
    TH_ENT_MO5,
    TH_EN_LGUI,
    TH_KC_LALT,
    TH_KC_LCTRL,
    NUM_TAP_HOLD_KEYS,
} tap_hold_key_id_t;

// --- レイヤー定義 ---
enum custom_layers {
    LAYER_0 = 0,
    LAYER_1,
    LAYER_2,
    LAYER_3,
    LAYER_4,
    LAYER_5,
};

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

#endif
