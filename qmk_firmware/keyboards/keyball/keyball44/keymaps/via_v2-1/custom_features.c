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

// --- ジェスチャー用移動量アキュムレータ ---
static int16_t gesture_x_accumulator = 0;
static int16_t gesture_y_accumulator = 0;

// --- タップ・ホールドキー識別子 ---
typedef enum { TH_BS_MO2, TH_EN_LGUI, TH_G_KEY, TH_KC_LALT, TH_KC_LCTRL, NUM_TAP_HOLD_KEYS } tap_hold_key_id_t;

// --- タップ・ホールドキー設定の配列 ---
static tap_hold_key_config_t tap_hold_keys[NUM_TAP_HOLD_KEYS] = {
    [TH_BS_MO2] = {.state = {0}, .tap_keycode = KC_ENT, .hold_target = LAYER_1, .hold_type = HOLD_TYPE_LAYER},
    [TH_EN_LGUI] = {.state = {0}, .tap_keycode = KC_LNG2, .hold_target = KC_LGUI, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_G_KEY] = {.state = {0}, .tap_keycode = KC_BTN1, .hold_target = KC_NO, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_KC_LALT] = {.state = {0}, .tap_keycode = KC_ESC, .hold_target = KC_LALT, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_KC_LCTRL] = {.state = {0}, .tap_keycode = KC_TAB, .hold_target = KC_LCTL, .hold_type = HOLD_TYPE_KEYCODE},
};

// --- ヘルパー関数 ---
static void enable_click_layer(void) {
    layer_on(LAYER_2);
    click_timer = timer_read();
    state = CLICKABLE;
}

static void disable_click_layer(void) {
    state = NONE;
    layer_off(LAYER_2);
}

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

// ホールドアクション有効化
// EN_LGUIのホールドはTap Dance内で直接register/unregisterするため、
// ここは他のTHキー専用として残します。
static void activate_hold_action(tap_hold_key_config_t *config) {
    if (config->state.active_for_hold) return;

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_on(config->hold_target);
    } else {
        register_code(config->hold_target);
    }
    config->state.active_for_hold = true;
    config->state.tap_action_done = true;  // ホールドが確定したらタップアクションは実行しない
}

// ホールドアクション無効化
// EN_LGUIのホールドはTap Dance内で直接register/unregisterするため、
// ここは他のTHキー専用として残します。
static void deactivate_hold_action(tap_hold_key_config_t *config) {
    if (!config->state.active_for_hold) return;

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_off(config->hold_target);
    } else {
        unregister_code(config->hold_target);
    }
    config->state.active_for_hold = false;
}

// タップアクション実行 (この関数はTH_EN_LGUIでは使われませんが、他のTHキーのために残します)fskajfk
static void perform_tap_action(tap_hold_key_config_t *config) {
    if (config->tap_keycode != KC_NO) {
        tap_code(config->tap_keycode);
    }
    config->state.tap_action_done = true;
}

// タップ・ホールドキー処理 (process_record_user から呼び出し)
// この関数は、キーイベントがカスタム処理によって「消費された」場合にfalseを返す。
static bool process_tap_hold_key(tap_hold_key_config_t *config, keyrecord_t *record) {
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
            if (timer_elapsed(config->state.timer) < TAPPING_TERM) {
                perform_tap_action(config);
            }
        }
    }
    return false;  // 基本的にカスタム処理で完結するのでfalseを返す
}

// タップ・ホールドキーのロールオーバー処理
static void check_tap_hold_rollover(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold && !config->state.tap_action_done) {
        if (timer_elapsed(config->state.timer) < TAPPING_TERM) {
            // 他のキーが押された瞬間に、このキーがタップ期間内であればホールドアクションを起動
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

// ジェスチャートリガーキーが押された/離されたときの共通処理
static void handle_gesture_trigger_key_state(bool pressed) {
    is_gesture_key_tapped = pressed;
    if (pressed) {
        mouse_action_cooldown_timer = timer_read() - MOUSE_ACTION_COOLDOWN_MS - 1;  // クールダウンを即時解除
        gesture_x_accumulator = 0;
        gesture_y_accumulator = 0;
        active_mouse_action = MOUSE_ACTION_STATE_NONE;
    } else {
        is_gesture_key_tapped = false;  // 明示的に false にする
        active_mouse_action = MOUSE_ACTION_STATE_NONE;
        mouse_action_cooldown_timer = timer_read();
        mouse_movement_accumulator = 0;
        gesture_x_accumulator = 0;
        gesture_y_accumulator = 0;
    }
}

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
                if (mouse_movement_accumulator >= CLICKABLE_MIN_MOVEMENT) {  // 定数を使
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
            tap_hold_keys[TH_KC_LALT].state.key_pressed ||
            tap_hold_keys[TH_KC_LALT].state.active_for_hold;  // 元のコードのkc_lalt_config.stateを参照
        bool g_is_gesture_trigger =
            tap_hold_keys[TH_G_KEY].state.key_pressed ||
            tap_hold_keys[TH_G_KEY].state.active_for_hold;  // 元のコードのg_key_config.stateを参照

        if (m_key_is_gesture_trigger || kc_lalt_is_gesture_trigger || g_is_gesture_trigger) {
            gesture_x_accumulator += current_x;
            gesture_y_accumulator += current_y;
        } else {
            gesture_x_accumulator = 0;
            gesture_y_accumulator = 0;
            active_mouse_action = MOUSE_ACTION_STATE_NONE;  // ジェスチャー状態もリセット
            mouse_action_cooldown_timer = timer_read();
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
                if (abs_acc_x + abs_acc_y >= GESTURE_MIN_ACCUMULATED_MOVEMENT) {  // 総移動量が閾値を超えた場合のみ判定
                    if (abs_acc_x > abs_acc_y * HORIZONTAL_SENSITIVITY_FACTOR) {
                        is_mostly_horizontal = true;
                    } else if (abs_acc_y > abs_acc_x * VERTICAL_SENSITIVITY_FACTOR) {
                        is_mostly_vertical = true;
                    }
                }

                if (is_mostly_horizontal) {
                    if (kc_lalt_is_gesture_trigger) {
                        unregister_code(KC_LALT);
                    }
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
                    if (kc_lalt_is_gesture_trigger) {
                        unregister_code(KC_LALT);
                    }
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
                    gesture_x_accumulator = 0;                      // アクション実行後にリセット
                    gesture_y_accumulator = 0;                      // アクション実行後にリセット
                    active_mouse_action = MOUSE_ACTION_STATE_NONE;  // 元のコードではここでもNONEにリセット
                }

            } else if (active_mouse_action != MOUSE_ACTION_STATE_NONE) {
                // ジェスチャー方向と逆の動きを検出したらリセット、または動きが止まったらリセット
                if ((active_mouse_action == MOUSE_ACTION_STATE_FOR_LEFT_SWIPE && current_x > 0) ||
                    (active_mouse_action == MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE && current_x < 0) ||
                    (active_mouse_action == MOUSE_ACTION_STATE_FOR_UP_SWIPE && current_y > 0) ||
                    (active_mouse_action == MOUSE_ACTION_STATE_FOR_DOWN_SWIPE && current_y < 0) ||
                    (current_x == 0 && current_y == 0)) {  // 元のコードの「動きが止まったらリセット」の条件
                    active_mouse_action = MOUSE_ACTION_STATE_NONE;
                    mouse_action_cooldown_timer = timer_read();
                    gesture_x_accumulator = 0;  // リセット
                    gesture_y_accumulator = 0;  // リセット
                }
            }

        } else {  // ジェスチャートリガーが押されていない場合 (マウスが動いている)
            // ジェスチャートリガーが離されたら状態をリセット
            if (active_mouse_action != MOUSE_ACTION_STATE_NONE || gesture_x_accumulator != 0 ||
                gesture_y_accumulator != 0) {
                active_mouse_action = MOUSE_ACTION_STATE_NONE;
                mouse_action_cooldown_timer = timer_read();
                gesture_x_accumulator = 0;  // リセット
                gesture_y_accumulator = 0;  // リセット
            }

            // マウスが動いていない場合のクリック状態の処理
            // (これは元のコードのelse if current_x == 0 && current_y == 0のブロックから移動)
            switch (state) {
                case CLICKABLE:
                    if (timer_elapsed(click_timer) > CLICKABLE_RESET_TIME) {
                        // disable_click_layer(); // 元々コメントアウト
                    }
                    break;

                case WAITING:
                    if (timer_elapsed(click_timer) > 50) {
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
                    // disable_click_layer(); // 元々コメントアウト
                }
                break;

            case WAITING:
                if (timer_elapsed(click_timer) > 50) {
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

//-- タップダンスの処理ここから ----------------

// EN_LGUIのタップダンス処理
void dance_en_lgui_finished(tap_dance_state_t *td_state, void *user_data) {
    if (td_state->pressed) {  // キーがまだ押されている（ホールドされている）場合
        register_code(KC_LGUI);
    } else {  // キーが離された（タップと判定された）場合
        if (td_state->count == 1) {
            unregister_code(KC_LGUI);

            // シングルタップでKC_LNG2
            register_code(KC_LNG2);
            unregister_code(KC_LNG2);
        } else if (td_state->count == 2) {
            unregister_code(KC_LGUI);

            register_code(KC_LCTL);
            register_code(KC_UP);
            unregister_code(KC_UP);
            unregister_code(KC_LCTL);
        }
    }
}

void dance_en_lgui_reset(tap_dance_state_t *td_state, void *user_data) {
    unregister_code(KC_LGUI);
    unregister_code(KC_LNG2);

    // if (layer_state_is(LAYER_5)) {
    //     layer_off(LAYER_5);
    // }
    // unregister_code(KC_ESC);
}

// BS_MO2 のタップダンス処理
void dance_bs_mo2_finished(tap_dance_state_t *td_state, void *user_data) {
    if (td_state->count == 1) {   // シングルタップまたはシングルホールド
        if (td_state->pressed) {  // キーがまだ押されている (ホールド状態)
            // シングルホールド: LAYER_1 を有効化 (MO2)
            layer_on(LAYER_1);
        } else {  // キーが離された (タップ状態)
            // シングルタップ: KC_BSPC を送信
            tap_code(KC_BSPC);
        }
    } else if (td_state->count == 2) {  // ダブルタップまたはダブルタップホールド
        if (td_state->pressed) {        // キーがまだ押されている (ホールド状態)
            // ダブルタップホールド: KC_BSPC を長押し
            register_code(KC_BSPC);
        } else {  // キーが離された (タップ状態)
            // ダブルタップ: KC_BSPC を送信
            tap_code(KC_BSPC);
        }
    }
}

void dance_bs_mo2_reset(tap_dance_state_t *td_state, void *user_data) {
    // ホールド中に押されていた可能性があるキーをすべて離す
    layer_off(LAYER_1);        // MO2 レイヤーをオフにする
    unregister_code(KC_BSPC);  // KC_BSPC の長押しを解除する
}

// Q
void dance_q_finished(tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        tap_code(KC_Q);
    } else {
        tap_code(KC_ESCAPE);
    }
}

void dance_q_reset(tap_dance_state_t *state, void *user_data) {}

tap_dance_action_t tap_dance_actions[] = {
    [TD_A_ESC] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_q_finished, dance_q_reset),
    [TD_EN_LGUI_LANG] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_en_lgui_finished, dance_en_lgui_reset),
    [TD_BS_MO1] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_bs_mo2_finished, dance_bs_mo2_reset),
};

// キーイベント処理
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // 他のキー入力があった場合に TH_KC_LALT のロールオーバーをチェック
    if (record->event.pressed) {
        check_tap_hold_rollover(&tap_hold_keys[TH_KC_LALT]);
        check_tap_hold_rollover(&tap_hold_keys[TH_BS_MO2]);
    }

    // マウスレイヤー中にマウスボタン以外のキーが押されたらレイヤーOFF
    if (layer_state_is(LAYER_2)) {
        switch (keycode) {
            case KC_BTN1:
            case KC_BTN2:
            case MO(LAYER_3):
                // これらのキーは CLICK_LAYER の状態に影響を与えない
                break;
            default:
                // それ以外のキーが押されたら
                if (record->event.pressed) {
                    // ただし、NAV_LAYER がアクティブでない場合のみ CLICK_LAYER を無効にする
                    if (!layer_state_is(LAYER_3)) {
                        disable_click_layer();
                    }
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
            break;
        }

        case KC_LALT:
            handle_gesture_trigger_key_state(record->event.pressed);
            return process_tap_hold_key(&tap_hold_keys[TH_KC_LALT], record);

        case BS_MO2:
            return process_tap_hold_key(&tap_hold_keys[TH_BS_MO2], record);
        case EN_LGUI:
            return process_tap_hold_key(&tap_hold_keys[TH_EN_LGUI], record);

        case GESTURE:
            handle_gesture_trigger_key_state(record->event.pressed);
            return process_tap_hold_key(&tap_hold_keys[TH_G_KEY], record);

            // case KC_LCTL:
            //     process_tap_hold_key(&tap_hold_keys[TH_KC_LCTRL], record);
            //     return true;

        default:
            // 他のキーが押されたらロールオーバー処理
            if (record->event.pressed) {
                for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
                    // TH_EN_LGUI は Tap Dance で処理されるため、ここから除外
                    // if (i != TH_EN_LGUI) {
                    check_tap_hold_rollover(&tap_hold_keys[i]);
                    // }
                }
            }

            break;
    }
    return true;  // 処理を継続するかどうかの最終決定
}

// キーボードスキャン処理
void matrix_scan_user(void) {
    // 各タップ・ホールドキーのホールド判定
    for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
        // TH_EN_LGUI は Tap Dance で処理されるため、ここではスキップ
        // if (i != TH_EN_LGUI) {
        matrix_scan_tap_hold_key(&tap_hold_keys[i]);
        // }
    }
}
