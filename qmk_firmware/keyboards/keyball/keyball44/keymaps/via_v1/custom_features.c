#include "custom_features.h"  // カスタム機能のヘッダー

#include "timer.h"  // タイマー関数用

// --- ジェスチャー状態管理 ---
static bool is_gesture_key_tapped = false;

// --- 自動マウスレイヤー関係 ---
static mouse_action_active_state_t active_mouse_action = MOUSE_ACTION_STATE_NONE;
static uint16_t mouse_action_cooldown_timer;  // クールダウンタイマー

// --- クリック状態管理 ---
static click_state_t state = NONE;
static uint16_t click_timer;
static int16_t mouse_movement_accumulator;  // マウス移動量蓄積

static void enable_click_layer(void) {
    layer_on(_CLICK_LAYER);
    click_timer = timer_read();
    state = CLICKABLE;
}

static void disable_click_layer(void) {
    state = NONE;
    layer_off(_CLICK_LAYER);
}

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

static bool other_key_pressed_while_tap_hold_pending = false;  // 他のキーが押されたフラグ

// タップ・ホールドキー設定
// 既存キー
static tap_hold_key_config_t kc_lalt_config = {
    .state = {0},
    .tap_keycode = KC_ESC,
    .hold_target = KC_LALT,
    .hold_type = HOLD_TYPE_KEYCODE,
};
static tap_hold_key_config_t kc_lctrl_config = {
    .state = {0},
    .tap_keycode = KC_TAB,
    .hold_target = KC_LCTL,
    .hold_type = HOLD_TYPE_KEYCODE,
};

// カスタムキー
static tap_hold_key_config_t jp_mo2_config = {
    .state = {0},
    .tap_keycode = KC_LNG1,
    .hold_target = _JP_MO2_LAYER,
    .hold_type = HOLD_TYPE_LAYER,
};
static tap_hold_key_config_t en_lgui_config = {
    .state = {0},
    .tap_keycode = KC_LNG2,
    .hold_target = KC_LGUI,
    .hold_type = HOLD_TYPE_KEYCODE,
};

// ジェスチャー関連
static tap_hold_key_config_t g_key_config = {
    .state = {0},
    .tap_keycode = KC_BTN1,
    .hold_target = KC_NO,
    .hold_type = HOLD_TYPE_KEYCODE,
};

// ホールドアクション有効化
static void activate_hold_action(tap_hold_key_config_t *config) {
    if (config->state.active_for_hold) return;

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_on(config->hold_target);
    } else {
        register_code(config->hold_target);
    }
    config->state.active_for_hold = true;
    config->state.tap_action_done = true;
}

// ホールドアクション無効化
static void deactivate_hold_action(tap_hold_key_config_t *config) {
    if (!config->state.active_for_hold) return;

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_off(config->hold_target);
    } else {
        unregister_code(config->hold_target);
    }
    config->state.active_for_hold = false;
}

// タップアクション実行
static void perform_tap_action(tap_hold_key_config_t *config) {
    if (config->tap_keycode != KC_NO) {
        tap_code(config->tap_keycode);
    }
    config->state.tap_action_done = true;
}

// タップ・ホールドキー処理 (process_record_user から呼び出し)
static bool process_tap_hold_key(tap_hold_key_config_t *config,
                                 keyrecord_t *record,
                                 bool is_other_key_pressed_for_tap) {
    if (record->event.pressed) {
        config->state.key_pressed = true;
        config->state.timer = timer_read();
        config->state.active_for_hold = false;
        config->state.tap_action_done = false;
    } else {  // キーが離された場合
        config->state.key_pressed = false;
        if (config->state.active_for_hold) {
            deactivate_hold_action(config);
        } else if (!config->state.tap_action_done) {
            // ホールドされておらず、かつタップアクションがまだ実行されていない場合
            // (TAPPING_TERM 以内に離された場合。matrix_scan_userでホールド判定される前)
            // other_key_pressed_while_tap_hold_pending は、このキーを離す前に他のキーが押されたかを見る
            if (timer_elapsed(config->state.timer) < TAPPING_TERM && !is_other_key_pressed_for_tap) {
                perform_tap_action(config);
            }
        }
    }
    return false;
}

// タップ・ホールドキーのロールオーバー処理
static void check_tap_hold_rollover(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold && !config->state.tap_action_done) {
        if (timer_elapsed(config->state.timer) < TAPPING_TERM) {
            activate_hold_action(config);
        }
    }
}

// タップ・ホールドキーのホールド判定 (matrix_scan_user から呼び出し)
static void matrix_scan_tap_hold_key(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold) {
        if (timer_elapsed(config->state.timer) > TAPPING_TERM) {
            activate_hold_action(config);
        }
    }
}

// static変数に追加
static int16_t gesture_x_accumulator = 0;
static int16_t gesture_y_accumulator = 0;

// ポインティングデバイスのタスク
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;
    report_mouse_t report_to_send = mouse_report;
    bool gesture_action_was_performed = false;

    // ジェスチャー用アキュムレータに現在の動きを加算
    gesture_x_accumulator += current_x;
    gesture_y_accumulator += current_y;

    if (current_x != 0 || current_y != 0) {
        switch (state) {
            case CLICKABLE:
                click_timer = timer_read();  // マウスが動いたらタイマー更新
                break;
            case WAITING:
                mouse_movement_accumulator += my_abs(current_x) + my_abs(current_y);
                if (mouse_movement_accumulator >= CLICKABLE_MIN_MOVEMENT) {  // 定数を使用
                    mouse_movement_accumulator = 0;
                    enable_click_layer();
                }
                break;
            default:
                click_timer = timer_read();
                state = WAITING;
                mouse_movement_accumulator = 0;
                break;
        }

        // ジェスチャー有効トリガー判定
        bool m_key_is_gesture_trigger = is_gesture_key_tapped;
        bool kc_lalt_is_gesture_trigger =
            kc_lalt_config.state.key_pressed || kc_lalt_config.state.active_for_hold || is_gesture_key_tapped;
        bool g_is_gesture_trigger =
            g_key_config.state.key_pressed || g_key_config.state.active_for_hold || is_gesture_key_tapped;

        // ジェスチャートリガーが有効な場合のみ、ジェスチャー用アキュムレータに現在の動きを加算
        if (m_key_is_gesture_trigger || kc_lalt_is_gesture_trigger || g_is_gesture_trigger) {
            gesture_x_accumulator += current_x;
            gesture_y_accumulator += current_y;
        } else {
            // ジェスチャートリガーキーがどれも押されていない場合、アキュムレータをクリア
            // これにより、不必要な蓄積を防ぎ、次のジェスチャーの開始時にクリーンな状態を保証
            gesture_x_accumulator = 0;
            gesture_y_accumulator = 0;
            active_mouse_action = MOUSE_ACTION_STATE_NONE;  // ジェスチャー状態もリセット
        }

        if (m_key_is_gesture_trigger) {
            gesture_action_was_performed = true;

            if (active_mouse_action == MOUSE_ACTION_STATE_NONE &&
                timer_elapsed(mouse_action_cooldown_timer) > MOUSE_ACTION_COOLDOWN_MS) {  // 定数を使用
                bool action_performed_in_this_cycle = false;
                bool is_mostly_horizontal = false;
                bool is_mostly_vertical = false;

                // 累積された移動量で方向を判定
                int16_t abs_acc_x = my_abs(gesture_x_accumulator);
                int16_t abs_acc_y = my_abs(gesture_y_accumulator);

                if (abs_acc_x > 0 || abs_acc_y > 0) {  // 何らかの動きがある場合にのみチェック
                    if (abs_acc_x > abs_acc_y * HORIZONTAL_SENSITIVITY_FACTOR) {  // 定数を使用
                        is_mostly_horizontal = true;
                    } else if (abs_acc_y > abs_acc_x * VERTICAL_SENSITIVITY_FACTOR) {  // 定数を使用
                        is_mostly_vertical = true;
                    }
                }

                if (is_mostly_horizontal) {
                    if (kc_lalt_is_gesture_trigger) unregister_code(KC_LALT);

                    if (g_is_gesture_trigger) {
                        /* nope */
                    }

                    // 累積されたX方向の動きで方向を決定
                    if (gesture_x_accumulator < 0) {
                        register_code(KC_LCTL);
                        tap_code(KC_RIGHT);
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_LEFT_SWIPE;
                        action_performed_in_this_cycle = true;
                    } else if (gesture_x_accumulator > 0) {
                        register_code(KC_LCTL);
                        tap_code(KC_LEFT);
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE;
                        action_performed_in_this_cycle = true;
                    }
                } else if (is_mostly_vertical) {
                    if (kc_lalt_is_gesture_trigger) unregister_code(KC_LALT);

                    if (g_is_gesture_trigger) {
                        /* nope */
                    }

                    // 累積されたY方向の動きで方向を決定
                    if (gesture_y_accumulator > 0) {
                        register_code(KC_LCTL);
                        tap_code(KC_DOWN);
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_UP_SWIPE;
                        action_performed_in_this_cycle = true;
                    } else if (gesture_y_accumulator < 0) {
                        register_code(KC_LCTL);
                        tap_code(KC_UP);
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_DOWN_SWIPE;
                        action_performed_in_this_cycle = true;
                    }
                }

                if (action_performed_in_this_cycle) {
                    mouse_action_cooldown_timer = timer_read();
                    gesture_x_accumulator = 0;  // アクション実行後にリセット
                    gesture_y_accumulator = 0;  // アクション実行後にリセット
                    active_mouse_action = MOUSE_ACTION_STATE_NONE;
                }
            } else if (active_mouse_action != MOUSE_ACTION_STATE_NONE) {
                // ジェスチャー方向と逆の動きを検出したらリセット、または動きが止まったらリセット
                if ((active_mouse_action == MOUSE_ACTION_STATE_FOR_LEFT_SWIPE && current_x > 0) ||
                    (active_mouse_action == MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE && current_x < 0) ||
                    (active_mouse_action == MOUSE_ACTION_STATE_FOR_UP_SWIPE && current_y > 0) ||
                    (active_mouse_action == MOUSE_ACTION_STATE_FOR_DOWN_SWIPE && current_y < 0) ||
                    (current_x == 0 && current_y == 0)) {
                    active_mouse_action = MOUSE_ACTION_STATE_NONE;
                    mouse_action_cooldown_timer = timer_read();
                    gesture_x_accumulator = 0;  // リセット
                    gesture_y_accumulator = 0;  // リセット
                }
            }
        } else {  // ジェスチャートリガーが押されていない場合
            // ジェスチャートリガーが離されたら状態をリセット
            if (active_mouse_action != MOUSE_ACTION_STATE_NONE || gesture_x_accumulator != 0 ||
                gesture_y_accumulator != 0) {
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
                mouse_action_cooldown_timer = timer_read();
                gesture_x_accumulator = 0;  // リセット
                gesture_y_accumulator = 0;  // リセット
            }
            // マウスが動いていない場合のクリック状態の処理
            switch (state) {
                case CLICKABLE:
                    if (timer_elapsed(click_timer) > CLICKABLE_RESET_TIME) {  // 定数を使用
                        // disable_click_layer(); // process_record_userで制御のためコメントアウト
                    }
                    break;
                case WAITING:
                    if (timer_elapsed(click_timer) > 50) {  // この50msも定数化を検討しても良いでしょう
                        mouse_movement_accumulator = 0;
                        state = NONE;
                    }
                    break;
                default:
                    break;
            }
        }
    } else {  // マウスが動いていない場合（current_xもcurrent_yも0）
        // ジェスチャーのアキュムレータをリセット
        if (gesture_x_accumulator != 0 || gesture_y_accumulator != 0) {
            gesture_x_accumulator = 0;
            gesture_y_accumulator = 0;
        }
        // マウスが動いていない場合のクリック状態の処理
        switch (state) {
            case CLICKABLE:
                if (timer_elapsed(click_timer) > CLICKABLE_RESET_TIME) {  // 定数を使用
                    // disable_click_layer(); // process_record_userで制御のためコメントアウト
                }
                break;
            case WAITING:
                if (timer_elapsed(click_timer) > 50) {  // この50msも定数化を検討しても良いでしょう
                    mouse_movement_accumulator = 0;
                    state = NONE;
                }
                break;
            default:
                break;
        }
    }

    report_to_send.v = -report_to_send.v;  // スクロール方向反転
    report_to_send.h = -report_to_send.h;  // スクロール方向反転

    if (gesture_action_was_performed) {  // ジェスチャー実行時はマウス移動をキャンセル
        report_to_send.x = 0;
        report_to_send.y = 0;
        report_to_send.v = 0;
        report_to_send.h = 0;
    }
    return report_to_send;
}

// キーイベント処理
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        other_key_pressed_while_tap_hold_pending = false;
    }

    // マウスレイヤー中にマウスボタン以外のキーが押されたらレイヤーOFF
    if (record->event.pressed && layer_state_is(_CLICK_LAYER)) {
        switch (keycode) {
            case KC_BTN1:
            case KC_BTN2:
            case MO(_NAV_LAYER):
                break;  // マウスボタンや関連キーは無視
            default:
                if (!layer_state_is(_NAV_LAYER)) {
                    disable_click_layer();
                }
                break;
        }
    }

    switch (keycode) {
        case KC_BTN1:
        case KC_BTN2: {
            report_mouse_t currentReport = pointing_device_get_report();
            uint8_t btn_mask = (keycode == KC_BTN1) ? MOUSE_BTN1 : MOUSE_BTN2;

            if (record->event.pressed) {
                currentReport.buttons |= btn_mask;
                state = CLICKING;
            } else {
                currentReport.buttons &= ~btn_mask;
                if (state == CLICKING) {
                    enable_click_layer();  // CLICKABLEに戻す
                }
            }
            pointing_device_set_report(currentReport);
            pointing_device_send();
            return false;
        }

        case JP_MO2:
            return process_tap_hold_key(&jp_mo2_config, record, other_key_pressed_while_tap_hold_pending);

        case EN_LGUI:
            process_tap_hold_key(&en_lgui_config, record, other_key_pressed_while_tap_hold_pending);
            if (record->event.pressed) {
                unregister_mods(MOD_LGUI);
            }
            return false;

        case GESTURE:
            if (record->event.pressed) {
                // クールダウンを即時解除して次のジェスチャーを可能にする
                is_gesture_key_tapped = true;
                mouse_action_cooldown_timer = timer_read() - MOUSE_ACTION_COOLDOWN_MS - 1;
                gesture_x_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
                gesture_y_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
            } else {
                is_gesture_key_tapped = false;
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
                mouse_action_cooldown_timer = timer_read();
                mouse_movement_accumulator = 0;
                gesture_x_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
                gesture_y_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
            }
            return process_tap_hold_key(&g_key_config, record, other_key_pressed_while_tap_hold_pending);

        case KC_LCTL:
            return process_tap_hold_key(&kc_lctrl_config, record, other_key_pressed_while_tap_hold_pending);

        case KC_LALT:
            if (record->event.pressed) {
                // クールダウンを即時解除して次のジェスチャーを可能にする
                is_gesture_key_tapped = true;
                mouse_action_cooldown_timer = timer_read() - MOUSE_ACTION_COOLDOWN_MS - 1;
                gesture_x_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
                gesture_y_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
            } else {
                is_gesture_key_tapped = false;
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
                mouse_action_cooldown_timer = timer_read();
                mouse_movement_accumulator = 0;
                gesture_x_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
                gesture_y_accumulator = 0;  // ジェスチャー用アキュムレータもリセット
            }
            return process_tap_hold_key(&kc_lalt_config, record, other_key_pressed_while_tap_hold_pending);

        default:
            // 他のキーが押されたらロールオーバー処理
            if (record->event.pressed) {
                other_key_pressed_while_tap_hold_pending = true;

                check_tap_hold_rollover(&jp_mo2_config);
                check_tap_hold_rollover(&en_lgui_config);
                check_tap_hold_rollover(&g_key_config);
                check_tap_hold_rollover(&kc_lalt_config);
                check_tap_hold_rollover(&kc_lctrl_config);
            }
            break;
    }
    return true;
}

// キーボードスキャン処理
void matrix_scan_user(void) {
    // 各タップ・ホールドキーのホールド判定
    matrix_scan_tap_hold_key(&jp_mo2_config);
    matrix_scan_tap_hold_key(&en_lgui_config);
    matrix_scan_tap_hold_key(&g_key_config);
    matrix_scan_tap_hold_key(&kc_lalt_config);
    matrix_scan_tap_hold_key(&kc_lctrl_config);

    // CLICKABLE状態のタイムアウト処理 (process_record_user で制御されるためコメントアウト)
    // if (state == CLICKABLE && timer_elapsed(click_timer) > CLICKABLE_RESET_TIME) { // 定数を使用
    //     // disable_click_layer();
    // }
}
