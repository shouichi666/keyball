#include "bs_mo5.h"

#include "feature_tap_hold/tap_hold.h"  // tap_hold_keys 参照のため

static uint16_t bs_mo5_timer = 0;
static uint8_t bs_mo5_tap_count = 0;
static bool bs_mo5_key_pressed = false;
static bool bs_mo5_bspc_hold_registered = false;
static bool bs_mo5_layer_hold_registered = false;

bool handle_bs_mo5_record(keyrecord_t *record) {
    if (record->event.pressed) {
        bs_mo5_key_pressed = true;

        if (timer_elapsed(bs_mo5_timer) < BS_MO2_DOUBLE_TAP_TERM) {
            bs_mo5_tap_count++;
        } else {
            bs_mo5_tap_count = 1;
        }
        bs_mo5_timer = timer_read();

        if (bs_mo5_tap_count == 2) {
            register_code(tap_hold_keys[TH_BS_MO5].tap_keycode);  // BSPC
            bs_mo5_bspc_hold_registered = true;
            bs_mo5_layer_hold_registered = false;
            layer_off(tap_hold_keys[TH_BS_MO5].hold_target);  // レイヤーOFF
        }

    } else {
        if (bs_mo5_bspc_hold_registered) {
            unregister_code(tap_hold_keys[TH_BS_MO5].tap_keycode);
            bs_mo5_bspc_hold_registered = false;
            bs_mo5_tap_count = 0;
        } else if (bs_mo5_layer_hold_registered) {
            layer_off(tap_hold_keys[TH_BS_MO5].hold_target);
            bs_mo5_layer_hold_registered = false;
        } else {
            if (bs_mo5_tap_count < 2) {
                tap_code(tap_hold_keys[TH_BS_MO5].tap_keycode);
            }
        }
        bs_mo5_key_pressed = false;
    }

    return true;  // イベント消費済み
}

void handle_bs_mo5_matrix_scan(void) {
    if (bs_mo5_key_pressed && bs_mo5_tap_count == 1 && !bs_mo5_layer_hold_registered && !bs_mo5_bspc_hold_registered) {
        if (timer_elapsed(bs_mo5_timer) > TAPPING_TERM) {
            layer_on(tap_hold_keys[TH_BS_MO5].hold_target);
            bs_mo5_layer_hold_registered = true;
        }
    }
}
