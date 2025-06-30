#pragma once

#include "features.h"
#include "quantum.h"

// --- 設定定数 ---
#define GESTURE_MIN_ACCUMULATED_MOVEMENT 160
#define MOUSE_ACTION_COOLDOWN_MS 15
#define HORIZONTAL_SENSITIVITY_FACTOR 2
#define VERTICAL_SENSITIVITY_FACTOR 2

// --- マウスジェスチャー状態 ---
typedef enum {
    MOUSE_ACTION_STATE_NONE = 0,
    MOUSE_ACTION_STATE_FOR_LEFT_SWIPE,
    MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE,
    MOUSE_ACTION_STATE_FOR_UP_SWIPE,
    MOUSE_ACTION_STATE_FOR_DOWN_SWIPE
} mouse_action_active_state_t;

// ジェスチャー関連処理
void handle_gesture_trigger_key_state(bool pressed);

// ポインティングデバイス処理のラッパー
report_mouse_t gesture_pointing_device_task(report_mouse_t mouse_report);
