#include "custom_features.h"  // 上で作成したヘッダーファイルをインクルード

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

// 各タップ・ホールドキーの設定
static tap_hold_key_config_t jp_mo2_config = {
    .state = {0},
    .tap_keycode = KC_LNG1,  // 日本語入力のトグル等を想定
    .hold_target = _JP_MO2_LAYER,
    .hold_type = HOLD_TYPE_LAYER,
};

static tap_hold_key_config_t en_lgui_config = {
    .state = {0},
    .tap_keycode = KC_LNG2,  // 英語入力のトグル等を想定
    .hold_target = KC_LGUI,
    .hold_type = HOLD_TYPE_KEYCODE,
};

// process_record_user 内で他のキーが押されたことを示すフラグ
static bool other_key_pressed_while_tap_hold_pending = false;

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
// https://github.com/kamiichi99/keyball/blob/main/qmk_firmware/keyboards/keyball/readme.md
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;

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
                mouse_movement_accumulator = 0;  // 蓄積値をリセット
                break;
        }
    } else {  // マウスが止まっている場合
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
                // WAITING状態で一定時間（50ms）マウスの動きがなければNONE状態に戻る
                // この50msは、微小な停止を無視するためのものか、あるいは即座にNONEに戻したくない場合の猶予時間
                if (timer_elapsed(click_timer) > 50) {  // 50msは仮の値。必要に応じて調整。
                    mouse_movement_accumulator = 0;     // 念のためリセット
                    state = NONE;
                }
                break;
            default:  // NONE の場合など
                // 特に行う処理なし
                break;
        }
    }

    return mouse_report;
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