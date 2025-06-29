#pragma once

#include "features.h"
#include "quantum.h"

// ジェスチャー関連処理
void handle_gesture_trigger_key_state(bool pressed);

// ポインティングデバイス処理のラッパー
report_mouse_t gesture_pointing_device_task(report_mouse_t mouse_report);
