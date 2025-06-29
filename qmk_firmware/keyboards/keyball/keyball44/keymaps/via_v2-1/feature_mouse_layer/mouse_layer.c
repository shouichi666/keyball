#include "mouse_layer.h"

static click_state_t state = NONE;
static uint16_t click_timer = 0;
static int16_t mouse_movement_accumulator = 0;

static int16_t my_abs(int16_t num) { return num < 0 ? -num : num; }

void enable_click_layer(void) {
    layer_on(LAYER_2);
    click_timer = timer_read();
    state = CLICKABLE;
}

void disable_click_layer(void) {
    state = NONE;
    layer_off(LAYER_2);
}

void update_mouse_layer_on_movement(int16_t x, int16_t y) {
    switch (state) {
        case CLICKABLE:
            click_timer = timer_read();  // 動いたらリセット
            break;
        case WAITING:
            mouse_movement_accumulator += my_abs(x) + my_abs(y);
            if (mouse_movement_accumulator >= CLICKABLE_MIN_MOVEMENT) {
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
}

void update_mouse_layer_on_idle(void) {
    switch (state) {
        case CLICKABLE:
            if (timer_elapsed(click_timer) > CLICKABLE_RESET_TIME) {
                // disable_click_layer(); // 必要であれば有効化
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

void handle_mouse_button(uint16_t keycode, bool pressed) {
    report_mouse_t currentReport = pointing_device_get_report();
    uint8_t btn_mask = (keycode == KC_BTN1) ? MOUSE_BTN1 : MOUSE_BTN2;

    if (pressed) {
        currentReport.buttons |= btn_mask;
        state = CLICKING;
    } else {
        currentReport.buttons &= ~btn_mask;
        if (state == CLICKING) {
            enable_click_layer();  // ボタン離したとき再びCLICKABLEへ
        }
    }

    pointing_device_set_report(currentReport);
    pointing_device_send();
}
