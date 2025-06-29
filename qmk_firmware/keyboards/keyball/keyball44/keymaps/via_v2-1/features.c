#include "features.h"

// 各モジュールのヘッダーをインクルード
#include "feature_custom_tap_dance/bs_mo5.h"
#include "feature_gesture/gesture.h"
#include "feature_mouse_layer/mouse_layer.h"
#include "feature_tap_dance/tap_dance.h"
#include "feature_tap_hold/tap_hold.h"

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // BS_MO5以外のキーが押された時、BS_MO5が押下中ならレイヤー確定
    if (record->event.pressed && keycode != BS_MO5) {
        if (tap_hold_keys[TH_BS_MO5].state.key_pressed) {
            layer_on(tap_hold_keys[TH_BS_MO5].hold_target);
            // BS_MO5のレイヤーホールド登録フラグをbs_mo5.cに持っていったので無視してよい
        }
    }

    if (record->event.pressed) {
        for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
            if (i != TH_BS_MO5) {
                check_tap_hold_rollover(&tap_hold_keys[i]);
            }
        }
    }

    if (layer_state_is(LAYER_2)) {
        switch (keycode) {
            case KC_BTN1:
            case KC_BTN2:
            case MO(LAYER_3):
                break;
            default:
                if (record->event.pressed && !layer_state_is(LAYER_3)) {
                    disable_click_layer();
                }
                break;
        }
    }

    switch (keycode) {
        case KC_BTN1:
        case KC_BTN2:
            handle_mouse_button(keycode, record->event.pressed);
            break;

        case KC_LALT:
            handle_gesture_trigger_key_state(record->event.pressed);
            return process_tap_hold_key(&tap_hold_keys[TH_KC_LALT], record);

        case KC_LCTL:
            return process_tap_hold_key(&tap_hold_keys[TH_KC_LCTRL], record);

        case ENT_MO4:
            handle_gesture_trigger_key_state(record->event.pressed);
            return process_tap_hold_key(&tap_hold_keys[TH_ENT_MO4], record);

        case BS_MO5:
            return handle_bs_mo5_record(record);

        case JP_MO2:
            return process_tap_hold_key(&tap_hold_keys[TH_JP_MO1], record);

        case EN_LGUI:
            return process_tap_hold_key(&tap_hold_keys[TH_EN_LGUI], record);

        case L_MO3:
            process_tap_hold_key(&tap_hold_keys[TH_L_MO3], record);
            if (!record->event.pressed) {
                layer_off(LAYER_3);
            }
            return true;

        case GESTURE:
            handle_gesture_trigger_key_state(record->event.pressed);
            return process_tap_hold_key(&tap_hold_keys[TH_G_KEY], record);

        default:
            if (record->event.pressed) {
                for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
                    if (i != TH_BS_MO5) {
                        check_tap_hold_rollover(&tap_hold_keys[i]);
                    }
                }
            }
            break;
    }

    return true;
}

void matrix_scan_user(void) {
    handle_bs_mo5_matrix_scan();

    for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
        if (i != TH_BS_MO5) {
            matrix_scan_tap_hold_key(&tap_hold_keys[i]);
        }
    }
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    update_mouse_layer_on_movement(mouse_report.x, mouse_report.y);

    report_mouse_t modified = gesture_pointing_device_task(mouse_report);

    if (mouse_report.x == 0 && mouse_report.y == 0) {
        update_mouse_layer_on_idle();
    }

    return modified;
}
