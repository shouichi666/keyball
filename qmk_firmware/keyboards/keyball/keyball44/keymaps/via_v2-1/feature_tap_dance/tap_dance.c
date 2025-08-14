#include "tap_dance.h"

void dance_en_lgui_finished(tap_dance_state_t *td_state, void *user_data) {
    if (td_state->pressed) {
        register_code(KC_LGUI);
    } else {
        if (td_state->count == 1) {
            tap_code(KC_LNG2);
        } else if (td_state->count == 2) {
            tap_code(KC_ESC);
        }
    }
}

void dance_en_lgui_reset(tap_dance_state_t *td_state, void *user_data) { unregister_code(KC_LGUI); }

void dance_btn2_lalt_finished(tap_dance_state_t *td_state, void *user_data) {
    if (td_state->pressed) {
        register_code(KC_LALT);
    } else {
        if (td_state->count == 1) {
            tap_code(KC_ESC);
        } else if (td_state->count == 2) {
        }
    }
}

void dance_btn2_lalt_reset(tap_dance_state_t *td_state, void *user_data) { unregister_code(KC_LALT); }

tap_dance_action_t tap_dance_actions[] = {
    // Tap Danceアクション配列
    [TD_EN_LGUI_LANG] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_en_lgui_finished, dance_en_lgui_reset),
    [TD_BTN2_LALT_LANG] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_btn2_lalt_finished, dance_btn2_lalt_reset),
};