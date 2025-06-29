#include "gesture.h"

// --- 内部状態 ---
static bool is_gesture_key_tapped = false;
static int16_t gesture_x_accumulator = 0;
static int16_t gesture_y_accumulator = 0;
static mouse_action_active_state_t active_mouse_action = MOUSE_ACTION_STATE_NONE;
static uint16_t mouse_action_cooldown_timer = 0;

// ヘルパー
static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

// 状態リセット
static void reset_gesture_state(void) {
    gesture_x_accumulator = 0;
    gesture_y_accumulator = 0;
    active_mouse_action = MOUSE_ACTION_STATE_NONE;
    mouse_action_cooldown_timer = timer_read();
}

// トリガーキー押下時処理
void handle_gesture_trigger_key_state(bool pressed) {
    is_gesture_key_tapped = pressed;
    if (pressed) {
        mouse_action_cooldown_timer = timer_read() - MOUSE_ACTION_COOLDOWN_MS - 1;
        gesture_x_accumulator = 0;
        gesture_y_accumulator = 0;
        active_mouse_action = MOUSE_ACTION_STATE_NONE;
    } else {
        reset_gesture_state();
    }
}

// ポインティングデバイス処理
report_mouse_t gesture_pointing_device_task(report_mouse_t mouse_report) {
    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;
    report_mouse_t report_to_send = mouse_report;

    gesture_x_accumulator += current_x;
    gesture_y_accumulator += current_y;

    bool gesture_triggered = is_gesture_key_tapped;

    if (gesture_triggered && active_mouse_action == MOUSE_ACTION_STATE_NONE &&
        timer_elapsed(mouse_action_cooldown_timer) > MOUSE_ACTION_COOLDOWN_MS) {
        bool is_mostly_horizontal = false;
        bool is_mostly_vertical = false;

        int16_t abs_x = my_abs(gesture_x_accumulator);
        int16_t abs_y = my_abs(gesture_y_accumulator);

        if (abs_x + abs_y >= GESTURE_MIN_ACCUMULATED_MOVEMENT) {
            if (abs_x > abs_y * HORIZONTAL_SENSITIVITY_FACTOR) {
                is_mostly_horizontal = true;
            } else if (abs_y > abs_x * VERTICAL_SENSITIVITY_FACTOR) {
                is_mostly_vertical = true;
            }
        }

        if (is_mostly_horizontal) {
            if (gesture_x_accumulator < 0) {
                register_code(KC_LCTL);
                tap_code(KC_RIGHT);
                unregister_code(KC_LCTL);
                active_mouse_action = MOUSE_ACTION_STATE_FOR_LEFT_SWIPE;
            } else {
                register_code(KC_LCTL);
                tap_code(KC_LEFT);
                unregister_code(KC_LCTL);
                active_mouse_action = MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE;
            }
        } else if (is_mostly_vertical) {
            if (gesture_y_accumulator > 0) {
                register_code(KC_LCTL);
                tap_code(KC_DOWN);
                unregister_code(KC_LCTL);
                active_mouse_action = MOUSE_ACTION_STATE_FOR_UP_SWIPE;
            } else {
                register_code(KC_LCTL);
                tap_code(KC_UP);
                unregister_code(KC_LCTL);
                active_mouse_action = MOUSE_ACTION_STATE_FOR_DOWN_SWIPE;
            }
        }

        if (is_mostly_horizontal || is_mostly_vertical) {
            gesture_x_accumulator = 0;
            gesture_y_accumulator = 0;
            mouse_action_cooldown_timer = timer_read();
        }

    } else if (active_mouse_action != MOUSE_ACTION_STATE_NONE) {
        // 逆方向や静止したらリセット
        if ((active_mouse_action == MOUSE_ACTION_STATE_FOR_LEFT_SWIPE && current_x > 0) ||
            (active_mouse_action == MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE && current_x < 0) ||
            (active_mouse_action == MOUSE_ACTION_STATE_FOR_UP_SWIPE && current_y > 0) ||
            (active_mouse_action == MOUSE_ACTION_STATE_FOR_DOWN_SWIPE && current_y < 0) ||
            (current_x == 0 && current_y == 0)) {
            reset_gesture_state();
        }
    }

    // スクロール方向反転
    report_to_send.v = -report_to_send.v;
    report_to_send.h = -report_to_send.h;

    if (gesture_triggered) {
        report_to_send.x = 0;
        report_to_send.y = 0;
        report_to_send.v = 0;
        report_to_send.h = 0;
    }

    return report_to_send;
}
