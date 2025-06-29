#pragma once

#include "features.h"
#include "quantum.h"

// マウスクリックレイヤー制御
void enable_click_layer(void);
void disable_click_layer(void);

// マウスレイヤー状態遷移（ポインティングデバイスイベント用）
void update_mouse_layer_on_movement(int16_t x, int16_t y);

// マウスが停止しているときの状態処理
void update_mouse_layer_on_idle(void);

// ボタン処理（BTN1/BTN2）
void handle_mouse_button(uint16_t keycode, bool pressed);
