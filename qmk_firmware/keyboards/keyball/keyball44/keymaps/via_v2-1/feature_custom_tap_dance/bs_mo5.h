#pragma once

#include "quantum.h"

#define BS_MO2_DOUBLE_TAP_TERM 150  // ダブルタップ判定時間

// BS_MO5キーの処理
bool handle_bs_mo5_record(keyrecord_t *record);

// BS_MO5キーのホールド判定（matrix_scan_user から呼び出し）
void handle_bs_mo5_matrix_scan(void);

void handle_bs_mo5_interrupt(void);
