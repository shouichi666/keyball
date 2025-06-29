#pragma once

#include "features.h"
#include "quantum.h"

// タップ・ホールドキー処理
bool process_tap_hold_key(tap_hold_key_config_t *config, keyrecord_t *record);

// ロールオーバー処理
void check_tap_hold_rollover(tap_hold_key_config_t *config);

// スキャン処理でのホールド判定
void matrix_scan_tap_hold_key(tap_hold_key_config_t *config);

// BS_MO5 を除くタップ・ホールドキーの定義と個数
extern tap_hold_key_config_t tap_hold_keys[];
#define NUM_TAP_HOLD_KEYS_DEFINED NUM_TAP_HOLD_KEYS
