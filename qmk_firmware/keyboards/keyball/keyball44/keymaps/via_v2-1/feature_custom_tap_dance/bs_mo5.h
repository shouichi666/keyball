#pragma once

#include "quantum.h"

// BS_MO5キーの処理
bool handle_bs_mo5_record(keyrecord_t *record);

// BS_MO5キーのホールド判定（matrix_scan_user から呼び出し）
void handle_bs_mo5_matrix_scan(void);
