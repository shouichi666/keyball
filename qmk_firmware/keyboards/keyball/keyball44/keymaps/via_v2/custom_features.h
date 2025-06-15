#ifndef CUSTOM_FEATURES_H
#define CUSTOM_FEATURES_H

// QMKの主要な定義をインクルード (report_mouse_t, keyrecord_t など)
#include QMK_KEYBOARD_H

// --- 設定可能な定数 ---
#define GESTURE_MIN_ACCUMULATED_MOVEMENT 65
#define MOUSE_ACTION_COOLDOWN_MS 100
#define HORIZONTAL_SENSITIVITY_FACTOR 2
#define VERTICAL_SENSITIVITY_FACTOR 2
#define CLICKABLE_RESET_TIME 800  // to_reset_timeをより分かりやすい名前に変更
#define CLICKABLE_MIN_MOVEMENT 5  // to_clickable_movementをより分かりやすい名前に変更

// Tap Dance専用の enum を定義
enum tap_dance_keycodes {
    TD_A_ESC = 0,
    TD_BS_MO1,
};

// 独自のキー
enum custom_keycodes {
    BS_MO2 = 0x9F00,  // tap: JP,   hold: レイヤー２,
    EN_LGUI,          // tap: EN,   hold: Command,
    GESTURE,
};

// レイヤーj
enum custom_layers {
    _LAYER_0 = 0,
    _LAYER_1,
    _LAYER_2,
    _LAYER_3,
    _LAYER_4,
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

// --- ホールドアクションの種類 ---
typedef enum {
    HOLD_TYPE_LAYER,
    HOLD_TYPE_KEYCODE,
} hold_action_type_t;

// --- タップ・ホールドの状態管理用構造体 ---k
typedef struct {
    bool key_pressed;      // キーが物理的に押されているか
    bool active_for_hold;  // ホールドアクションが有効になっているか
    uint16_t timer;        // キーが押された時刻
    bool tap_action_done;  // タップアクションが実行済みか
} tap_hold_state_t;

// --- タップ・ホールドキーの設定構造体 ---
typedef struct {
    tap_hold_state_t state;        // キーの現在の状態
    uint16_t tap_keycode;          // タップ時に送信するキーコード
    uint16_t hold_target;          // ホールド時に有効化するレイヤー番号または送信するキーコード
    hold_action_type_t hold_type;  // ホールドアクションの種類
} tap_hold_key_config_t;

#endif
