# RGBLIGHT_ENABLE = yes

OLED_ENABLE = yes

VIA_ENABLE = yes

# 独自処理をビルド設定に追加
SRC += custom_features.c

# COMBOを有効化
COMBO_ENABLE = yes

# Tap Danceを有効
TAP_DANCE_ENABLE = yes

PMW3360_CPI = 3200 # 例: CPIを3200に設定 (デフォルトは1600や800の場合が多い)