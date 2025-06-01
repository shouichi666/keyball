#ifndef CUSTOM_FEATURES_H
#define CUSTOM_FEATURES_H

#include QMK_KEYBOARD_H  // QMKの主要な定義をインクルード (report_mouse_t, keyrecord_t など)

// 独自のキー
enum custom_keycodes {
    JP_MO2,    // 単: 日本語変換、長押し: レイヤー２
    EN_LGUI,   // 単: ローマ字変換、長押し: Command
    MINS_MO2,  // 単: -、長押し:  レイヤー２
    BS_MO2,    // 単: BS、長押し:  レイヤー２
};

enum custom_layers {
    _BASE_LAYER = 0,  // _LAYER_0 から変更 (より一般的な名称に)
    _JP_MO2_LAYER,    // _LAYER_1 (JP_MO2ホールド時のレイヤー)
    _CLICK_LAYER,     // _LAYER_2 (click_layerに対応するレイヤーとして命名)
    _NAV_LAYER,       // _LAYER_3 (現状使われていないように見えるため)
};

#endif
