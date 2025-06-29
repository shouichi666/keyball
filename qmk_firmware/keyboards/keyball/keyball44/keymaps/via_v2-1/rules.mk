# RGBLIGHT_ENABLE = yes

OLED_ENABLE = yes

VIA_ENABLE = yes

# COMBOを有効化
COMBO_ENABLE = yes

# Tap Danceを有効
TAP_DANCE_ENABLE = yes

PMW3360_CPI = 3200 # 例: CPIを3200に設定 (デフォルトは1600や800の場合が多い)

# 独自処理をビルド設定に追加
SRC += features.c \
       feature_tap_hold/tap_hold.c \
       feature_gesture/gesture.c \
       feature_mouse_layer/mouse_layer.c \
       feature_tap_dance/tap_dance.c \
       feature_custom_tap_dance/bs_mo5.c