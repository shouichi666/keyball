#include "tap_hold.h"

tap_hold_key_config_t tap_hold_keys[NUM_TAP_HOLD_KEYS_DEFINED] = {
    [TH_BS_MO5] = {.state = {0}, .tap_keycode = KC_BSPC, .hold_target = LAYER_5, .hold_type = HOLD_TYPE_LAYER},
    [TH_JP_MO1] = {.state = {0}, .tap_keycode = KC_LNG1, .hold_target = LAYER_1, .hold_type = HOLD_TYPE_LAYER},
    [TH_ENT_MO5] = {.state = {0}, .tap_keycode = KC_ENT, .hold_target = LAYER_5, .hold_type = HOLD_TYPE_LAYER},
    [TH_QUOT_MO4] = {.state = {0}, .tap_keycode = KC_QUOT, .hold_target = KC_NO, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_EN_LGUI] = {.state = {0}, .tap_keycode = KC_LNG2, .hold_target = KC_LGUI, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_G_KEY] = {.state = {0}, .tap_keycode = KC_BTN1, .hold_target = KC_NO, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_KC_LALT] = {.state = {0}, .tap_keycode = KC_BTN2, .hold_target = KC_LALT, .hold_type = HOLD_TYPE_KEYCODE},
    [TH_KC_LCTRL] = {.state = {0}, .tap_keycode = KC_TAB, .hold_target = KC_LCTL, .hold_type = HOLD_TYPE_KEYCODE},
};

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

static void deactivate_hold_action(tap_hold_key_config_t *config) {
    if (!config->state.active_for_hold) return;

    if (config->hold_type == HOLD_TYPE_LAYER) {
        layer_off(config->hold_target);
    } else {
        unregister_code(config->hold_target);
    }
    config->state.active_for_hold = false;
}

static void perform_tap_action(tap_hold_key_config_t *config) {
    if (config->tap_keycode != KC_NO) {
        tap_code(config->tap_keycode);
    }
    config->state.tap_action_done = true;
}

bool process_tap_hold_key(tap_hold_key_config_t *config, keyrecord_t *record) {
    if (record->event.pressed) {
        config->state.key_pressed = true;
        config->state.timer = timer_read();
        config->state.active_for_hold = false;
        config->state.tap_action_done = false;
    } else {
        config->state.key_pressed = false;
        if (config->state.active_for_hold) {
            deactivate_hold_action(config);
        } else if (!config->state.tap_action_done) {
            if (timer_elapsed(config->state.timer) < TAPPING_TERM) {
                perform_tap_action(config);
            }
        }
    }
    return false;
}

void check_tap_hold_rollover(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold && !config->state.tap_action_done) {
        if (timer_elapsed(config->state.timer) < TAPPING_TERM) {
            activate_hold_action(config);
        }
    }
}

void matrix_scan_tap_hold_key(tap_hold_key_config_t *config) {
    if (config->state.key_pressed && !config->state.active_for_hold) {
        if (timer_elapsed(config->state.timer) > TAPPING_TERM) {
            activate_hold_action(config);
        }
    }
}
