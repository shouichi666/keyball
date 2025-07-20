/*
This is the c configuration file for the keymap

Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// キーコードを送信する際のディレイ（ミリ秒単位）
#define TAP_CODE_DELAY 0

// 次のタップが来るのを待つ期間
#define TD_TAP_INTERVAL 4

// タップとホールドを判定する時間の閾値（ミリ秒単位）
#define TAPPING_TERM 150

// コンボの発動時間
#define COMBO_TERM 40

// 自動マウス移動機能を有効にする（QMKのポイントデバイス機能の一部）
#define POINTING_DEVICE_AUTO_MOUSE_ENABLE

// 自動マウス機能を有効にするレイヤーを指定（1番レイヤーが対象になる）
#define AUTO_MOUSE_DEFAULT_LAYER 2

// レイヤーの数を設定
#define DYNAMIC_KEYMAP_LAYER_COUNT 6

// マウス速度 (default: 500)
#define KEYBALL_CPI_DEFAULT 1500

// スクロール速度 (default: 4)
#define KEYBALL_SCROLL_DIV_DEFAULT 5

// タップとホールドの判定において、ホールドを強制する（素早いタップでもホールドと認識されやすくなる）
#define QUICK_TAP_TERM 4

// タップがあったときにホールドを待たずにそのキーが入力されるようにする
// (タップキーが押されて TAPPING_TERM 以内に離されれば、他のキーが押されてもタップとして機能する)
// #define NO_TAPPING_FORCE_HOLD

// // タップとホールドの判定において、次のキー入力があった場合はホールドと判定する（早めにホールドを有効にする）
#define PERMISSIVE_H