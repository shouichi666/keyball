#include "custom_features.h"  // 上で作成したヘッダーファイルをインクルード

#include "timer.h"  // タイマー関数を使用するために追加

// --- Gesture State Handling ---
static bool is_en_lgui_tapped = false;

// New: マウスジェスチャーの状態を示すためのEnum
typedef enum {
    MOUSE_ACTION_STATE_NONE = 0,         // 0: なし (元の0に相当)
    MOUSE_ACTION_STATE_FOR_LEFT_SWIPE,   // 左スワイプに対するアクション実行中 (元の1に相当: current_x < 0 の時)
    MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE,  // 右スワイプに対するアクション実行中 (元の-1に相当: current_x > 0 の時)
    MOUSE_ACTION_STATE_FOR_UP_SWIPE,     // New: 上スワイプに対するアクション実行中
    MOUSE_ACTION_STATE_FOR_DOWN_SWIPE    // New: 下スワイプに対するアクション実行中
} mouse_action_active_state_t;

// Modified: active_mouse_action の型を新しいEnumに変更
static mouse_action_active_state_t active_mouse_action =
    MOUSE_ACTION_STATE_NONE;                  // 0: なし, 1: RIGHT系アクション実行中, -1:
                                              // LEFT系アクション実行中　(このコメントは元の型の説明として残します)
static uint16_t mouse_action_cooldown_timer;  // アクションのクールダウン用タイマー
static const uint16_t MOUSE_ACTION_COOLDOWN_MS = 200;  // クールダウン時間(ms)、調整可能
// ★「ほぼ真横」判定のための係数。大きいほど、より真横に近い動きでないと反応しない
// 例: 2 ならX軸の動きがY軸の2倍以上、3 なら3倍以上必要。
static const int16_t HORIZONTAL_SENSITIVITY_FACTOR = 4;
// New: ★「ほぼ真縦」判定のための係数。大きいほど、より真縦に近い動きでないと反応しない
static const int16_t VERTICAL_SENSITIVITY_FACTOR = 4;

// --- Click State Handling ---
typedef enum {
    NONE = 0,
    WAITING,
    CLICKABLE,
    CLICKING,
} click_state_t;

static click_state_t state = NONE;
static uint16_t click_timer;
static const uint16_t to_reset_time = 800;       // CLICKABLE状態でマウスの動きが止まってからNONEに戻るまでの時間(ms)
static const int16_t to_clickable_movement = 0;  // WAITING状態からCLICKABLE状態に移行するために必要なマウスの移動量
// static const uint16_t click_layer = 1; // マウスクリック時に有効になるレイヤー番号 (custom_layers の _CLICK_LAYER
// を使用するよう変更)
static int16_t mouse_movement_accumulator;  // mouse_movement から変更 (より明確な名前に)

static void enable_click_layer(void) {
    layer_on(_CLICK_LAYER);  // click_layer 変数の代わりに _CLICK_LAYER を使用
    click_timer = timer_read();
    state = CLICKABLE;
}

static void disable_click_layer(void) {
    state = NONE;
    layer_off(_CLICK_LAYER);  // click_layer 変数の代わりに _CLICK_LAYER を使用
}

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

// --- Tap/Hold Key Handling ---
// タップ・ホールドの状態管理用構造体
typedef struct {
    bool key_pressed;      // 物理的にキーが押されているか
    bool active_for_hold;  // ホールドアクションが有効になっているか
    uint16_t timer;        // キーが押された時刻
    bool tap_action_done;  // タップアクションが実行済みか
                           // (ロールオーバー時の重複実行防止や、ホールド後のタップ防止)
} tap_hold_state_t;

// ホールドアクションの種類
typedef enum {
    HOLD_TYPE_LAYER,    // ホールドでレイヤーを有効化
    HOLD_TYPE_KEYCODE,  // ホールドでキーコードを送信
} hold_action_type_t;

// タップ・ホールドキーの設定構造体
typedef struct {
    tap_hold_state_t state;        // キーの現在の状態
    uint16_t tap_keycode;          // タップ時に送信するキーコード
    uint16_t hold_target;          // ホールド時に有効化するレイヤー番号または送信するキーコード
    hold_action_type_t hold_type;  // ホールドアクションの種類
} tap_hold_key_config_t;

// process_record_user 内で他のキーが押されたことを示すフラグ
static bool other_key_pressed_while_tap_hold_pending = false;

// 'JP_MO2'のタップ・ホールドキー設定
static tap_hold_key_config_t jp_mo2_config = {
    .state = {0},
    .tap_keycode = KC_LNG1,  // 日本語入力のトグル等を想定
    .hold_target = _JP_MO2_LAYER,
    .hold_type = HOLD_TYPE_LAYER,
};

// 'EN_LGUI'のタップ・ホールドキー設定
static tap_hold_key_config_t en_lgui_config = {
    .state = {0},
    .tap_keycode = KC_LNG2,  // 英語入力のトグル等を想定
    .hold_target = KC_LGUI,
    .hold_type = HOLD_TYPE_KEYCODE,
};

// ホールドアクションを有効化するヘルパー関数
static void activate_hold_action(tap_hold_key_config_t *config) {
    if (config->state.active_for_hold) return;  // 既にホールド状態なら何もしない

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_on(config->hold_target);
    } else {  // HOLD_TYPE_KEYCODE
        register_code(config->hold_target);
    }
    config->state.active_for_hold = true;
    config->state.tap_action_done = true;  // ホールドが有効になったらタップアクションは実行しない
}

// ホールドアクションを無効化するヘルパー関数
static void deactivate_hold_action(tap_hold_key_config_t *config) {
    if (!config->state.active_for_hold) return;  // ホールド状態でなければ何もしない

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_off(config->hold_target);
    } else {  // HOLD_TYPE_KEYCODE
        unregister_code(config->hold_target);
    }
    config->state.active_for_hold = false;
}

// タップアクションを実行するヘルパー関数
static void perform_tap_action(tap_hold_key_config_t *config) {
    if (config->tap_keycode != KC_NO) {  // KC_NOなら何もしない
        register_code(config->tap_keycode);
        unregister_code(config->tap_keycode);
    }
    config->state.tap_action_done = true;
}

// タップ・ホールドキーの処理を行う関数 (process_record_user から呼び出される)
static bool process_tap_hold_key(tap_hold_key_config_t *config,
                                 keyrecord_t *record,
                                 bool is_other_key_pressed_for_tap) {
    if (record->event.pressed) {
        config->state.key_pressed = true;
        config->state.timer = timer_read();
        config->state.active_for_hold = false;  // 押下時にはホールド状態をリセット
        config->state.tap_action_done = false;  // タップアクションもリセット
    } else {                                    // キーが離された場合
        config->state.key_pressed = false;
        if (config->state.active_for_hold) {  // ホールドとして処理されていた場合
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
    // QMKにこのキーイベントをここで処理完了と伝える
    return false;
}

// タップ・ホールドキーのロールオーバー処理 (他のキー入力によってホールドを確定させる)
static void check_tap_hold_rollover(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold && !config->state.tap_action_done) {
        // キーが押されていて、まだホールド判定されておらず、タップも未実行の場合
        if (timer_elapsed(config->state.timer) < TAPPING_TERM) {
            // TAPPING_TERM以内ならホールドとみなし、アクションを有効化
            activate_hold_action(config);
        }
    }
}

// タップ・ホールドキーのホールド判定 (matrix_scan_user から呼び出される)
static void matrix_scan_tap_hold_key(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold) {
        // キーが押され続けていて、まだホールドとして処理されていない場合 (ロールオーバーでもホールドにならなかった場合)
        if (timer_elapsed(config->state.timer) > TAPPING_TERM) {
            activate_hold_action(config);
        }
    }
}
// --- End of Tap/Hold Key Handling ---

// 自動レイヤーの設定
// 参考リポジトリ: https://github.com/kamiichi99/keyball/blob/main/qmk_firmware/keyboards/keyball/readme.md
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;
    // 返却するマウスレポートを準備 (初期値は受け取ったレポート)
    report_mouse_t report_to_send = mouse_report;
    // この関数呼び出しでジェスチャーが実行されたかどうかを示すフラグ
    bool gesture_action_was_performed = false;

    if (current_x != 0 || current_y != 0) {  // マウスが動いた場合
        switch (state) {
            case CLICKABLE:
                // CLICKABLE状態でマウスが動いたら、リセットタイマーを更新
                click_timer = timer_read();
                // 必要であれば、ここで disable_click_layer()
                // を呼び、即座にレイヤーを無効化することも検討できます
                break;
            case CLICKING:
                // マウスボタン押下中は特に何もしない
                break;
            case WAITING:
                mouse_movement_accumulator += my_abs(current_x) + my_abs(current_y);
                if (mouse_movement_accumulator >= to_clickable_movement) {
                    mouse_movement_accumulator = 0;
                    enable_click_layer();
                }
                break;
            default:  // NONE の場合など
                // マウスが動き始めたらWAITING状態に移行し、タイマーを開始
                click_timer = timer_read();
                state = WAITING;
                mouse_movement_accumulator = 0;
                break;
        }

        if (is_en_lgui_tapped &&
            en_lgui_config.state.active_for_hold) {  // EN_LGUIが物理的に押され、かつホールドが確定している
            // Modified: active_mouse_action のチェックを新しいEnum定数に変更
            if (active_mouse_action == MOUSE_ACTION_STATE_NONE &&
                timer_elapsed(mouse_action_cooldown_timer) > MOUSE_ACTION_COOLDOWN_MS) {
                bool action_performed_in_this_cycle = false;
                bool lgui_temporarily_unregistered = false;
                bool is_mostly_horizontal = false;
                bool is_mostly_vertical = false;  // New: 縦方向ジェスチャー判定用フラグ

                // ★「ほぼ真横」の判定ロジック
                if (current_x != 0) {      // X軸方向に動きがあることが前提
                    if (current_y == 0) {  // 完全に真横の動き
                        is_mostly_horizontal = true;
                    } else {
                        // X軸の移動量が、Y軸の移動量 * 係数 よりも大きいか判定
                        if (my_abs(current_x) > my_abs(current_y) * HORIZONTAL_SENSITIVITY_FACTOR) {
                            is_mostly_horizontal = true;
                        }
                    }
                }

                // New: ★「ほぼ真縦」の判定ロジック (横方向ジェスチャーが優先される)
                if (!is_mostly_horizontal &&
                    current_y != 0) {      // 横方向のジェスチャーが判定されなかった場合のみ縦方向を評価
                    if (current_x == 0) {  // 完全に真縦の動き
                        is_mostly_vertical = true;
                    } else {
                        // Y軸の移動量が、X軸の移動量 * 係数 よりも大きいか判定
                        if (my_abs(current_y) > my_abs(current_x) * VERTICAL_SENSITIVITY_FACTOR) {
                            is_mostly_vertical = true;
                        }
                    }
                }

                if (is_mostly_horizontal) {  // 「ほぼ真横」と判定された場合のみジェスチャー実行
                    // macOS向け: EN_LGUIのホールドターゲットがKC_LGUIの場合、一時的に無効化
                    if (en_lgui_config.hold_target == KC_LGUI) {
                        unregister_code(KC_LGUI);
                        lgui_temporarily_unregistered = true;
                    }

                    if (current_x < 0) {  // カーソルが左方向に動いている
                        register_code(KC_LCTL);
                        tap_code(KC_RIGHT);  // マウス左移動でCtrl+KC_RIGHT (macOSではCtrl+Right Arrow)
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_LEFT_SWIPE;  // Modified: Enum値を設定
                        action_performed_in_this_cycle = true;
                        gesture_action_was_performed = true;
                    } else if (current_x > 0) {  // カーソルが右方向に動いている
                        register_code(KC_LCTL);
                        tap_code(KC_LEFT);  // マウス右移動でCtrl+KC_LEFT (macOSではCtrl+Left Arrow)
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE;  // Modified: Enum値を設定
                        action_performed_in_this_cycle = true;
                        gesture_action_was_performed = true;
                    }
                    // New: 縦方向ジェスチャーの処理ブロック
                } else if (is_mostly_vertical) {  // 「ほぼ真縦」と判定された場合のみジェスチャー実行
                    // macOS向け: EN_LGUIのホールドターゲットがKC_LGUIの場合、一時的に無効化
                    // (もし縦ジェスチャーでも同様の対応が必要な場合)
                    if (en_lgui_config.hold_target == KC_LGUI) {
                        unregister_code(KC_LGUI);
                        lgui_temporarily_unregistered = true;
                    }

                    // カーソルが上方向に動いている
                    if (current_y < 0) {
                        // New: 上方向のジェスチャーアクション (例: Page Up)
                        // 必要に応じてアクションを変更してください (例: Mission Controlなど)
                        // register_code(KC_LCTL); tap_code(KC_UP); unregister_code(KC_LCTL);
                        register_code(KC_LCTL);
                        tap_code(KC_UP);
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_UP_SWIPE;  // New: Enum値を設定
                        action_performed_in_this_cycle = true;
                        gesture_action_was_performed = true;
                    } else if (current_y > 0) {  // カーソルが下方向に動いている
                        // New: 下方向のジェスチャーアクション (例: Page Down)
                        // 必要に応じてアクションを変更してください (例: Application Windowsなど)
                        // register_code(KC_LCTL); tap_code(KC_DOWN); unregister_code(KC_LCTL);
                        register_code(KC_LCTL);
                        tap_code(KC_DOWN);
                        unregister_code(KC_LCTL);
                        active_mouse_action = MOUSE_ACTION_STATE_FOR_DOWN_SWIPE;  // New: Enum値を設定
                        action_performed_in_this_cycle = true;
                        gesture_action_was_performed = true;
                    }
                }

                // 一時的に無効化したKC_LGUIを再有効化 (EN_LGUIがまだホールドされている場合のみ)
                if (lgui_temporarily_unregistered) {
                    if (is_en_lgui_tapped && en_lgui_config.state.active_for_hold) {
                        register_code(KC_LGUI);
                    }
                }

                if (action_performed_in_this_cycle) {
                    mouse_action_cooldown_timer = timer_read();
                }
            }
            // Modified: active_mouse_action のリセットロジック (Enumと上下方向に対応)
            else if (active_mouse_action != MOUSE_ACTION_STATE_NONE) {
                if (current_x == 0 && current_y == 0) {  // 水平および垂直の動きが止まった
                    active_mouse_action = MOUSE_ACTION_STATE_NONE;
                } else if ((active_mouse_action == MOUSE_ACTION_STATE_FOR_LEFT_SWIPE &&
                            current_x > 0) ||  // 左ブロック中に右へ
                           (active_mouse_action == MOUSE_ACTION_STATE_FOR_RIGHT_SWIPE &&
                            current_x < 0) ||  // 右ブロック中に左へ
                           (active_mouse_action == MOUSE_ACTION_STATE_FOR_UP_SWIPE &&
                            current_y > 0) ||  // New: 上ブロック中に下へ
                           (active_mouse_action == MOUSE_ACTION_STATE_FOR_DOWN_SWIPE &&
                            current_y < 0)  // New: 下ブロック中に上へ
                ) {
                    active_mouse_action = MOUSE_ACTION_STATE_NONE;
                }
            }
        } else {  // EN_LGUI がタップされていない、またはホールドがアクティブでない場合
            if (active_mouse_action != MOUSE_ACTION_STATE_NONE) {  // Modified: Enum定数で比較
                active_mouse_action = MOUSE_ACTION_STATE_NONE;     // Modified: Enum定数を設定
                if (!is_en_lgui_tapped) {                          // 物理的に離されたらクールダウンもリセット
                    mouse_action_cooldown_timer = timer_read();
                }
            }
        }
    } else {  // マウスが止まっている場合 (current_x == 0 && current_y == 0)
        if (active_mouse_action != MOUSE_ACTION_STATE_NONE) {  // Modified: Enum定数で比較 // ブロックされていたら解除
            active_mouse_action = MOUSE_ACTION_STATE_NONE;     // Modified: Enum定数を設定
            mouse_action_cooldown_timer = timer_read();        // マウス停止時もクールダウンをリセット
        }
        switch (state) {
            case CLICKING:
                // マウスボタンが離された際の処理は KC_MY_BTN の
                // process_record_user で行われるため、ここでは何もしない
                break;
            case CLICKABLE:
                // [NOTE]：マウスのクリック操作以外をタップするとマウスレイヤーをOFFにするため以下をコメントアウトしている
                // ---
                // CLICKABLE状態で一定時間マウスの動きがなければレイヤーを無効化
                // if (timer_elapsed(click_timer) > to_reset_time) {
                //     disable_click_layer();
                // }
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

    // ★ジェスチャーがこのサイクルで実行された場合、マウスカーソルの移動をキャンセル
    if (gesture_action_was_performed) {
        report_to_send.x = 0;
        report_to_send.y = 0;
        // ホイール移動もキャンセル
        report_to_send.v = 0;
        report_to_send.h = 0;
    }

    return report_to_send;
}

// 特定キーコードの動作をユーザー定義で上書きする処理
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // 他のキーが押されたことをリセット
    // このフラグは、タップ・ホールドキーが「タップ」として成立するかどうかの判定に使われる
    if (record->event.pressed) {
        other_key_pressed_while_tap_hold_pending = false;
    }

    // マウスレイヤー中にマウスボタン以外のキーが押されたらレイヤーOFF＆状態リセット
    if (record->event.pressed && layer_state_is(_CLICK_LAYER)) {
        switch (keycode) {
            case KC_BTN1:
            case KC_BTN2:
            case MO(_NAV_LAYER):
            case LCTL(KC_TAB):
            case RCS(KC_TAB):
                // マウスボタンや関連の修飾キーは何もしない
                break;
            default:
                if (!layer_state_is(_NAV_LAYER)) {
                    // _NAV_LAYER以外で上記のマウス操作以外のキーが押されたらマウスレイヤーOFFで状態リセットも行う
                    disable_click_layer();
                }

                break;
        }
    }

    switch (keycode) {
        case KC_BTN1:
        case KC_BTN2: {
            report_mouse_t currentReport = pointing_device_get_report();
            uint8_t btn_mask = (keycode == KC_BTN1) ? MOUSE_BTN1 : MOUSE_BTN2;  // QMKの定義に合わせて修正

            if (record->event.pressed) {
                currentReport.buttons |= btn_mask;
                state = CLICKING;  // マウスボタン押下中はCLICKING状態
            } else {
                currentReport.buttons &= ~btn_mask;
                // マウスボタンを離したらCLICKABLE状態に戻す
                // (pointing_device_task_user のロジックと合わせて調整が必要な場合あり)
                // 即座にCLICKABLEに戻すか、マウスが動くまでWAITINGにするかは設計次第
                if (state == CLICKING) {   // CLICKING状態からのみ遷移
                    enable_click_layer();  // CLICKABLE状態に戻しタイマーをリセット
                }
            }
            pointing_device_set_report(currentReport);
            pointing_device_send();  // マウスレポートを送信 (QMK v20以降では不要な場合あり、要確認)
            return false;            // このキーの処理はここで完了
        }

        case JP_MO2:
            return process_tap_hold_key(&jp_mo2_config, record, other_key_pressed_while_tap_hold_pending);

        case EN_LGUI:
            if (record->event.pressed) {
                is_en_lgui_tapped = true;
                // 押された瞬間はクールダウンタイマーをリセットして即座のアクションを許可
                mouse_action_cooldown_timer =
                    timer_read() - MOUSE_ACTION_COOLDOWN_MS - 1;  // 即座にタイムアウトするように調整
            } else {
                is_en_lgui_tapped = false;
                active_mouse_action = MOUSE_ACTION_STATE_NONE;  // Modified: Enum定数を設定
                mouse_action_cooldown_timer = timer_read();     // EN_LGUIを離したらタイマーもリセット
            }

            return process_tap_hold_key(&en_lgui_config, record, other_key_pressed_while_tap_hold_pending);

        default:
            // JP_MO2 または EN_LGUI が押されている間に他のキーが押された場合の処理 (ロールオーバー)
            if (record->event.pressed) {
                // 他のキーが押されたことを記録
                // (このフラグは、離されたタップホールドキーがタップとして振る舞うかを決定するために使われる)
                other_key_pressed_while_tap_hold_pending = true;

                // 各タップ・ホールドキーのロールオーバー処理を呼び出す
                check_tap_hold_rollover(&jp_mo2_config);
                check_tap_hold_rollover(&en_lgui_config);
            }
            break;
    }

    // 上記以外のキーは通常の処理を継続
    return true;
}

// QMKがキーの押下をチェックするたび実行される処理
void matrix_scan_user(void) {
    // 各タップ・ホールドキーのホールド判定
    matrix_scan_tap_hold_key(&jp_mo2_config);
    matrix_scan_tap_hold_key(&en_lgui_config);

    // --- Click State Handling (timer based) ---
    // pointing_device_task_user でマウスの動きがない場合のタイムアウト処理もここで行うことができる
    // (現在の実装では pointing_device_task_user 内で処理されている)
    // 例：
    if (state == CLICKABLE && timer_elapsed(click_timer) > to_reset_time) {
        // [NOTE]：マウスのクリック操作以外をタップするとマウスレイヤーをOFFにするため
        // process_record_user側で対応しており、ここでは自動OFFはコメントアウトのまま。
        // もし、キー入力がなくても一定時間でOFFにしたい場合は以下のコメントを外す。
        // disable_click_layer();
    } else if (state == WAITING && timer_elapsed(click_timer) > 50) {  // マウスが止まっている場合のタイムアウト
        // pointing_device_task_user内のマウス停止時のWAITING処理と重複する可能性あり。
        // どちらで管理するか明確にすると良い。ここではpointing_device_task_userに任せる。
        // mouse_movement_accumulator = 0;
        // state = NONE;
    }
}