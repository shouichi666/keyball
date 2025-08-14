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

// ALTキーによる左右ジェスチャーモードが有効かを示すフラグ
static bool is_alt_horizontal_gesture_mode = false;

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

static void reset_gesture_state(void) {
    is_gesture_key_tapped = false;
    active_mouse_action = MOUSE_ACTION_STATE_NONE;
    mouse_action_cooldown_timer = timer_read();
    gesture_x_accumulator = 0;
    gesture_y_accumulator = 0;
    is_alt_horizontal_gesture_mode = false;
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

    // クールダウン中は何もしない
    if (timer_elapsed(mouse_action_cooldown_timer) <= MOUSE_ACTION_COOLDOWN_MS) {
        report_to_send.v = -report_to_send.v;
        report_to_send.h = -report_to_send.h;

        // ジェスチャーモード中はマウスカーソルを止める
        if (gesture_modifier_held || is_alt_horizontal_gesture_mode) {
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
        int16_t abs_x = my_abs(gesture_x_accumulator);
        int16_t abs_y = my_abs(gesture_y_accumulator);
        bool is_mostly_horizontal = abs_x > abs_y * HORIZONTAL_SENSITIVITY_FACTOR;
        bool is_mostly_vertical = abs_y > abs_x * VERTICAL_SENSITIVITY_FACTOR;

        // 【変更点】ここから
        //  ALT + 上下ジェスチャーを最優先で処理する
        //  閾値なしで、最初の縦の動きに反応してレイヤーをONにする
        if (is_td_lalt_physically_down && !is_alt_horizontal_gesture_mode && !is_mouse_layer_active_by_gesture &&
            is_mostly_vertical && abs_y > 0) {
            layer_on(LAYER_3);
            is_mouse_layer_active_by_gesture = true;

            // 他のジェスチャーが発動しないようにアクション済み状態にする
            active_mouse_action = MOUSE_ACTION_STATE_FOR_UP_SWIPE;

            // クールダウンを開始し、アキュムレーターをリセット
            mouse_action_cooldown_timer = timer_read();
            gesture_x_accumulator = 0;
            gesture_y_accumulator = 0;
        }
        // 【変更点】ここまで
        //  ALT + 左右ジェスチャーモード（固定後）の処理
        else if (is_td_lalt_physically_down && is_alt_horizontal_gesture_mode) {
            // このモードでは左右の動きのみをジェスチャーとして認識する
            if (my_abs(gesture_x_accumulator) >= GESTURE_MIN_ACCUMULATED_MOVEMENT) {
                unregister_code(KC_LALT);
                register_code(KC_LGUI);
                register_code(KC_LALT);

                if (gesture_x_accumulator < 0) {
                    tap_code(KC_LEFT);
                } else {
                    tap_code(KC_RIGHT);
                }

                unregister_code(KC_LGUI);
                unregister_code(KC_LALT);

                // 次のジェスチャーのためにリセット
                gesture_x_accumulator = 0;
                gesture_y_accumulator = 0;
                mouse_action_cooldown_timer = timer_read();
            }
            // 縦の動きは完全に無視する
        }
        // 通常のジェスチャー処理（最初のジェスチャーまたはCTRLジェスチャー）
        else if (active_mouse_action == MOUSE_ACTION_STATE_NONE) {
            if (abs_x + abs_y >= GESTURE_MIN_ACCUMULATED_MOVEMENT) {
                // ALTキーの場合（水平方向のみ。垂直は上で処理済み）
                if (is_td_lalt_physically_down) {
                    if (is_mostly_horizontal) {
                        unregister_code(KC_LALT);
                        // 左右ジェスチャーを検知 -> モードを固定
                        is_alt_horizontal_gesture_mode = true;

                        register_code(KC_LGUI);
                        register_code(KC_LALT);
                        if (gesture_x_accumulator < 0) {
                            tap_code(KC_LEFT);
                            active_mouse_action = MOUSE_ACTION_STATE_FOR_LEFT_SWIPE;
                        } else {
                            tap_code(KC_RIGHT);
                            active_mouse_action = MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE;
                        }
                        unregister_code(KC_LGUI);
                        unregister_code(KC_LALT);
                    }
                }
                // CTRLキーの場合
                else if (is_tap_ctrl_down) {
                    unregister_code(KC_LCTL);
                    register_code(KC_LCTL);
                    if (is_mostly_horizontal) {
                        if (gesture_x_accumulator < 0) {
                            tap_code(KC_RIGHT);
                            active_mouse_action = MOUSE_ACTION_STATE_FOR_LEFT_SWIPE;
                        } else {
                            tap_code(KC_LEFT);
                            active_mouse_action = MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE;
                        }
                    } else if (is_mostly_vertical) {
                        if (gesture_y_accumulator > 0) {
                            tap_code(KC_DOWN);
                            active_mouse_action = MOUSE_ACTION_STATE_FOR_UP_SWIPE;
                        } else {
                            tap_code(KC_UP);
                            active_mouse_action = MOUSE_ACTION_STATE_FOR_DOWN_SWIPE;
                        }
                    }
                    unregister_code(KC_LCTL);
                }

                // ジェスチャー発火後の共通処理
                mouse_action_cooldown_timer = timer_read();
                gesture_x_accumulator = 0;
                gesture_y_accumulator = 0;
            }

            // マウスレイヤーがアクティブな場合、水平ジェスチャーでOFFにする
            if (is_mouse_layer_active_by_gesture && is_mostly_horizontal) {
                layer_off(LAYER_3);
                is_mouse_layer_active_by_gesture = false;
                reset_gesture_state();  // OFFにしたら状態を完全にリセット
            }
        } else {
            // ジェスチャーが一度発火した後のサイクル
            // ALT+左右モードでなければ状態をリセット
            if (!is_alt_horizontal_gesture_mode) {
                reset_gesture_state();
            } else {
                // ALT+左右モードの時は、active_mouse_actionのみリセットしてモードを継続
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
            }
        }
    } else {
        // 修飾キーが離されたらリセット
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
    if ((gesture_modifier_held && !is_mouse_layer_active_by_gesture) || is_alt_horizontal_gesture_mode) {
        report_to_send.x = 0;
        report_to_send.y = 0;
        report_to_send.v = 0;
        report_to_send.h = 0;
    }

    return report_to_send;
}