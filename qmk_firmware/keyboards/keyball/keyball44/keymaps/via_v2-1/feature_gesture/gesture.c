#include "gesture.h"

#include "feature_tap_hold/tap_hold.h"

bool is_mouse_layer_active_by_gesture = false;
extern bool is_td_lgui_physically_down;
extern bool is_td_lalt_physically_down;
static bool is_gesture_key_tapped = false;
static int16_t gesture_x_accumulator = 0;
static int16_t gesture_y_accumulator = 0;
static mouse_action_active_state_t active_mouse_action = MOUSE_ACTION_STATE_NONE;
static uint16_t mouse_action_cooldown_timer = 0;

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

static void reset_gesture_state(void) {
    is_gesture_key_tapped = false;
    active_mouse_action = MOUSE_ACTION_STATE_NONE;
    mouse_action_cooldown_timer = timer_read();
    gesture_x_accumulator = 0;
    gesture_y_accumulator = 0;
}

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

report_mouse_t gesture_pointing_device_task(report_mouse_t mouse_report) {
    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;
    report_mouse_t report_to_send = mouse_report;

    bool is_tap_ctrl_down =
        (tap_hold_keys[TH_KC_LCTRL].state.key_pressed || tap_hold_keys[TH_KC_LCTRL].state.active_for_hold);

    bool gesture_modifier_held = is_tap_ctrl_down || is_td_lalt_physically_down;

    // クールダウン中は何もしない（新規発火禁止）
    if (timer_elapsed(mouse_action_cooldown_timer) <= MOUSE_ACTION_COOLDOWN_MS) {
        // スクロール反転のみ行い、そのまま返す
        report_to_send.v = -report_to_send.v;
        report_to_send.h = -report_to_send.h;

        if (gesture_modifier_held) {
            report_to_send.x = 0;
            report_to_send.y = 0;
            report_to_send.v = 0;
            report_to_send.h = 0;
        }
        return report_to_send;
    }

    gesture_x_accumulator += current_x;
    gesture_y_accumulator += current_y;

    if (gesture_modifier_held) {
        if (active_mouse_action == MOUSE_ACTION_STATE_NONE) {
            int16_t abs_x = my_abs(gesture_x_accumulator);
            int16_t abs_y = my_abs(gesture_y_accumulator);
            bool is_mostly_horizontal = abs_x > abs_y * HORIZONTAL_SENSITIVITY_FACTOR;
            bool is_mostly_vertical = abs_y > abs_x * VERTICAL_SENSITIVITY_FACTOR;

            if (abs_x + abs_y >= GESTURE_MIN_ACCUMULATED_MOVEMENT) {
                if (is_td_lalt_physically_down) unregister_code(KC_LALT);
                if (is_tap_ctrl_down) unregister_code(KC_LCTL);
                // if (is_td_lgui_physically_down) unregister_code(KC_LGUI);

                if (is_mostly_horizontal) {
                    if (gesture_x_accumulator < 0) {
                        if (is_td_lalt_physically_down) {
                            register_code(KC_LGUI);
                            register_code(KC_LALT);
                            tap_code(KC_LEFT);
                            unregister_code(KC_LGUI);
                            unregister_code(KC_LALT);
                        } else {
                            register_code(KC_LCTL);
                            tap_code(KC_RIGHT);
                            unregister_code(KC_LCTL);
                        }
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_LEFT_SWIPE;
                    } else {
                        if (is_td_lalt_physically_down) {
                            register_code(KC_LALT);
                            register_code(KC_LGUI);
                            tap_code(KC_RIGHT);
                            unregister_code(KC_LGUI);
                            unregister_code(KC_LALT);
                        } else {
                            register_code(KC_LCTL);
                            tap_code(KC_LEFT);
                            unregister_code(KC_LCTL);
                        }
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE;
                    }
                } else if (is_mostly_vertical) {
                    if (gesture_y_accumulator > 0) {
                        if (is_td_lalt_physically_down) {
                            // nope
                        } else {
                            register_code(KC_LCTL);
                            tap_code(KC_DOWN);
                            unregister_code(KC_LCTL);
                        }
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_UP_SWIPE;
                    } else {
                        if (is_td_lalt_physically_down) {
                            // nope
                        } else {
                            register_code(KC_LCTL);
                            tap_code(KC_UP);
                            unregister_code(KC_LCTL);
                        }
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_DOWN_SWIPE;
                    }
                }

                // 発火直後は状態リセットしない。クールダウンをスタート
                mouse_action_cooldown_timer = timer_read();
                gesture_x_accumulator = 0;
                gesture_y_accumulator = 0;
            }

            if (is_mostly_vertical) {
                if (gesture_y_accumulator > 0) {
                    if (is_td_lalt_physically_down) {
                        layer_on(LAYER_3);
                        is_mouse_layer_active_by_gesture = true;
                    }
                } else {
                    if (is_td_lalt_physically_down) {
                        layer_on(LAYER_3);
                        is_mouse_layer_active_by_gesture = true;
                    }
                }
            }

            if (is_mostly_horizontal && is_mouse_layer_active_by_gesture) {
                layer_off(LAYER_3);
                is_mouse_layer_active_by_gesture = false;
            }
        } else {
            reset_gesture_state();
        }
    } else {
        if (is_mouse_layer_active_by_gesture) {
            layer_off(LAYER_3);
            is_mouse_layer_active_by_gesture = false;
        }
        reset_gesture_state();
    }

    // スクロール反転
    report_to_send.v = -report_to_send.v;
    report_to_send.h = -report_to_send.h;

    // ジェスチャー有効時はマウス移動・スクロールキャンセル
    if (gesture_modifier_held && !is_mouse_layer_active_by_gesture) {
        report_to_send.x = 0;
        report_to_send.y = 0;
        report_to_send.v = 0;
        report_to_send.h = 0;
    }

    return report_to_send;
}