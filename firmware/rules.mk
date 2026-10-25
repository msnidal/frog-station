VIA_ENABLE = yes
VIAL_ENABLE = yes
OLED_ENABLE = yes
EXTRAKEY_ENABLE = yes
MOUSEKEY_ENABLE = yes
WPM_ENABLE = yes
TAP_DANCE_ENABLE = yes
COMMAND_ENABLE = no
CONSOLE_ENABLE = no

QMK_SETTINGS = yes
LTO_ENABLE = yes

KEY_OVERRIDE_ENABLE = yes
DYNAMIC_MACRO_ENABLE = yes
SRC += station.c station_logic.c

# Preserve modern Vial capabilities from the installed original firmware.
COMBO_ENABLE = yes
REPEAT_KEY_ENABLE = yes
LAYER_LOCK_ENABLE = yes
GRAVE_ESC_ENABLE = yes
MAGIC_ENABLE = yes
BOOTMAGIC_ENABLE = yes
CAPS_WORD_ENABLE = yes

SRC += station_display.c
