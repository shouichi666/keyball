/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
hbut WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
''''
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "custom_features.h"  // 作成したヘッダーファイルをインクルード-
#include "quantum.h"

// clang-format off
// make SKIP_GIT=yes keyball/keyball44:via_v1
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [LAYER_0] = LAYOUT_universal(
        KC_TAB , KC_Q       , KC_W     , KC_E     , KC_R     , KC_T     ,                                        KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_QUOT   ,
        KC_LCTL, MT(MOD_RSFT,KC_A),KC_S, KC_D     , MT(MOD_LCTL,KC_F)   , KC_G     ,                             KC_H     , KC_J     , KC_K     , KC_L     , MT(MOD_RSFT, KC_SCLN), KC_MINS,
        KC_LSFT, KC_Z       , KC_X     , KC_C     , KC_V     , KC_B     ,                                        KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , KC_EQL    ,
                              KC_BTN2  , MO(4)  , KC_LALT  , TD(TD_EN_LGUI_LANG) , KC_SPC ,                   KC_ENT , BS_MO2,       RCTL_T(KC_LNG2) , KC_RALT  , LT(5, KC_BTN2)
    ),

    [LAYER_1] = LAYOUT_universal(
        _______  , KC_1     , KC_2     , KC_3     , KC_4     , KC_5      ,                                       KC_7     , KC_8     , KC_UP     , KC_9     , KC_0     , KC_GRV    ,
        _______  , KC_EXLM  , KC_AT    , KC_HASH  , KC_DLR   , KC_PERC   ,                                       KC_6     , KC_LEFT  , KC_DOWN   , KC_RIGHT , KC_LBRC  , KC_RBRC   ,
        _______  , KC_QUES  , KC_AMPR  , KC_LPRN  , KC_RPRN  , KC_PIPE   ,                                       KC_LPRN  , KC_RPRN  , _______   , _______  , _______  , KC_BSLS   ,
                              _______  , _______  ,  _______ , _______ , _______ ,                    _______ , _______ ,       _______       , _______  , _______
    ),

    [LAYER_2] = LAYOUT_universal(
        _______ ,  _______ ,  _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______ , _______   ,
        _______ ,  _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , KC_BTN1  , MO(LAYER_3), _______ , _______ ,
        _______ ,  _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______ , _______ ,
                              _______  , _______  ,  _______ , _______ , _______ ,                    _______ , _______ ,       _______       , _______  , _______
    ),

    [LAYER_3] = LAYOUT_universal(
        _______ ,  KC_F1    , KC_F2    , KC_F3    , KC_F4    , _______      ,                                    QK_BOOT  , _______     , _______    , _______    , _______  , _______  ,
        _______ ,  KC_F5    , KC_F6    , KC_F7    , KC_F8    , LAG(KC_LEFT) ,                                    _______  , LAG(KC_RIGHT), _______   , _______    , _______  , _______  ,
        _______ ,  KC_F9    , KC_F10   , KC_F11   , KC_F12   , LAG(KC_RIGHT),                                    _______  , LAG(KC_LEFT), _______    , _______    , _______  , _______  ,
                              QK_BOOT  , _______  , _______  , _______ , _______ ,                       LGUI(KC_W) , LGUI(KC_LEFT) ,          _______      , _______  , _______
    ),

    [LAYER_4] = LAYOUT_universal(
        _______ ,  KC_F1    , KC_F2    , KC_F3    , KC_F4    , _______   ,                                         _______      , KC_7      , KC_8   , KC_9     , KC_0      , _______  ,
        _______ ,  KC_F5    , KC_F6    , KC_F7    , KC_F8    , LAG(KC_LEFT) ,                                      LCTL(KC_RIGHT), KC_4      , KC_5   , KC_6     , KC_DOT    , _______  ,
        _______ ,  KC_F9    , KC_F10   , KC_F11   , KC_F12   , LAG(KC_RIGHT),                                      LCTL(KC_LEFT) , KC_1      , KC_2   , KC_3     , KC_COMM   , _______  ,
                              QK_BOOT  , _______  , _______  , _______ , _______ ,                       LGUI(KC_BSPC) , LALT(KC_BSPC) ,       _______      , _______  , _______
    ),

    [LAYER_5] = LAYOUT_universal(
        _______ ,  KC_F1    , KC_F2    , KC_F3    , KC_F4    , _______      ,                                      _______      , _______  , _______  , _______  , _______  , _______  ,
        _______ ,  KC_F5    , KC_F6    , KC_F7    , KC_F8    , LAG(KC_LEFT) ,                                      LCTL(KC_RIGHT), _______  , _______  , _______  , _______  , _______  ,
        _______ ,  KC_F9    , KC_F10   , KC_F11   , KC_F12   , LAG(KC_RIGHT),                                      LCTL(KC_LEFT) , _______  , _______  , _______  , _______  , _______  ,
                              QK_BOOT  , _______  , _______  , _______ , _______ ,                       LGUI(KC_BSPC) , LALT(KC_BSPC) ,       _______      , _______  , _______
    ),
};
// clang-format onn

//-- タップダンスの処理ここから ----------------

//-- コンボで使用するキーコードの記載 -----------
const uint16_t PROGMEM combo_jk[] = {KC_J, KC_K, COMBO_END};
const uint16_t PROGMEM combo_fd[] = {KC_F, KC_D, COMBO_END};
const uint16_t PROGMEM combo_sp[] = {KC_LALT, BS_MO2, COMBO_END};

// コンボの定義
combo_t key_combos[] = {
    COMBO(combo_jk, MO(4)),
    COMBO(combo_fd, MO(4)),
    COMBO(combo_sp, MO(3)),
};

uint16_t combo_size = ARRAY_SIZE(key_combos);

layer_state_t layer_state_set_user(layer_state_t state) {
    // Auto enable scroll mode when the highest layer is 3
    keyball_set_scroll_mode(get_highest_layer(state) >= 3);
    return state;
}

#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
