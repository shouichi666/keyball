#include "features.h"

// 各モジュールのヘッダーをインクルード
#include "feature_custom_tap_dance/bs_mo5.h"
#include "feature_gesture/gesture.h"
#include "feature_mouse_layer/mouse_layer.h"
#include "feature_tap_dance/tap_dance.h"
#include "feature_tap_hold/tap_hold.h"

bool is_td_lgui_physically_down = false;
bool is_td_lalt_physically_down = false;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // BS_MO1以外のキーが押された時、BS_MO1が押下中ならレイヤー確定
    if (record->event.pressed && keycode != BS_MO1) {
        handle_bs_mo5_interrupt();
    }

    if (record->event.pressed) {
        for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
            if (i != BS_MO1) {
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

        case TD(TD_EN_LGUI_LANG):
            if (record->event.pressed) {
                is_td_lgui_physically_down = true;
            } else {
                is_td_lgui_physically_down = false;
            }
            // tap-dance の通常の処理を妨げないように true を返す
            return true;

        case TD(TD_BTN2_LALT_LANG):
            if (record->event.pressed) {
                is_td_lalt_physically_down = true;
            } else {
                is_td_lalt_physically_down = false;
            }
            // tap-dance の通常の処理を妨げないように true を返す
            return true;

        case KC_LALT:
            handle_gesture_trigger_key_state(record->event.pressed);
            return process_tap_hold_key(&tap_hold_keys[TH_KC_LALT], record);

        case KC_LCTL:
            return process_tap_hold_key(&tap_hold_keys[TH_KC_LCTRL], record);

        case ENT_SFT:
            return process_tap_hold_key(&tap_hold_keys[TH_ENT_SFT], record);

        case BS_MO1:
            return handle_bs_mo5_record(record);

        case JP_MO1:
            return process_tap_hold_key(&tap_hold_keys[TH_JP_MO1], record);

        case EN_LGUI:
            return process_tap_hold_key(&tap_hold_keys[TH_EN_LGUI], record);

        default:
            if (record->event.pressed) {
                for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
                    if (i != BS_MO1) {
                        check_tap_hold_rollover(&tap_hold_keys[i]);
                    }
                }
            }
            break;
    }

    return true;
}

void matrix_scan_user(void) {
    for (int i = 0; i < NUM_TAP_HOLD_KEYS; ++i) {
        matrix_scan_tap_hold_key(&tap_hold_keys[i]);
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
