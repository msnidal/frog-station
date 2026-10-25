// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H
#include "transactions.h"
#include "atomic_util.h"
#include "station_logic.h"
#include "station_assets.h"
#include "station_display.h"
#include <stdio.h>
#include <string.h>

#if defined(INIT_EE_HANDS_LEFT) == defined(INIT_EE_HANDS_RIGHT)
#    error "Build for exactly one physical half: INIT_EE_HANDS_LEFT or INIT_EE_HANDS_RIGHT"
#endif
_Static_assert(OLED_DISPLAY_WIDTH == 128 && OLED_DISPLAY_HEIGHT == 32, "This artwork requires a 128x32 physical display rotated to portrait");
static fs_logic_t station;
static uint8_t drawn_frame = 255;
static uint8_t drawn_layer = 255, drawn_mods = 255;
static bool drawn_lamp, drawn_alert, diagnostic_drawn;
static uint8_t drawn_meter = 255, drawn_music_phase = 255;

static void receive_station(uint8_t in_length, const void *in, uint8_t out_length, void *out) {
    (void)out_length; (void)out;
    uint32_t now = sync_timer_read32();
    ATOMIC_BLOCK_RESTORESTATE { fs_unpack_host(&station, in, in_length, now); }
}
void keyboard_post_init_user(void) {
    transaction_register_rpc(FS_HOST_SYNC, receive_station);
}
void housekeeping_task_user(void) {
    // Coalesce small timestamp/status packets; never send keycodes or pixels.
    static uint32_t last_rpc;
    static uint8_t previous[FS_WIRE_SIZE];
    if (!is_keyboard_master() || timer_elapsed32(last_rpc) < 50) return;
    uint8_t wire[FS_WIRE_SIZE];
    ATOMIC_BLOCK_RESTORESTATE { fs_pack_host(&station, wire); }
    if (memcmp(previous, wire, sizeof(wire)) == 0 && timer_elapsed32(last_rpc) < 500) return;
    last_rpc = timer_read32();
    if (transaction_rpc_send(FS_HOST_SYNC, sizeof(wire), wire)) memcpy(previous, wire, sizeof(wire));
}
static uint8_t typing_mods(void) {
    uint8_t mods = get_mods() | get_weak_mods();
#ifndef NO_ACTION_ONESHOT
    mods |= get_oneshot_mods() | get_oneshot_locked_mods();
#endif
    return mods;
}
static bool text_key(uint16_t kc) {
    return (kc >= KC_A && kc <= KC_0) || (kc >= KC_SPC && kc <= KC_SLSH) ||
           (kc >= KC_PSLS && kc <= KC_PMNS) || kc == KC_PPLS ||
           (kc >= KC_P1 && kc <= KC_PDOT) || kc == KC_NUBS || kc == KC_PEQL;
}
static uint16_t typing_keycode(uint16_t kc) {
    if (typing_mods() & (MOD_MASK_CTRL | MOD_MASK_ALT | MOD_MASK_GUI)) return KC_NO;
    if (IS_QK_MODS(kc)) {
        if (QK_MODS_GET_MODS(kc) & (MOD_LCTL | MOD_LALT | MOD_LGUI)) return KC_NO;
        return kc & 0xff;
    }
    return kc;
}
bool wpm_keycode_user(uint16_t keycode) {
    // Dual-role keys are counted only after tap/hold has been resolved below.
    return text_key(typing_keycode(keycode));
}
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed || !is_keyboard_master()) return true;
    bool dual = IS_QK_LAYER_TAP(keycode) || IS_QK_MOD_TAP(keycode);
    if (dual) {
        if (!record->tap.count) return true;
        keycode &= 0xff;
    }
    keycode = typing_keycode(keycode);
    if (text_key(keycode) || keycode == KC_BSPC || keycode == KC_ENT || keycode == KC_TAB || keycode == KC_PENT) {
        uint32_t now = sync_timer_read32();
        ATOMIC_BLOCK_RESTORESTATE { fs_note_typing(&station, now); }
        if (dual && text_key(keycode)) update_wpm(keycode);
    }
    return true;
}
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint32_t now = sync_timer_read32();
    bool handled;
    ATOMIC_BLOCK_RESTORESTATE { handled = fs_handle_hid(&station, data, length, now); }
    if (!handled && length) data[0] = 0xFF;
    // VIA owns the reply and calls raw_hid_send after this hook returns.
}
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    (void)rotation;
    return is_keyboard_left() ? FS_LEFT_ROTATION : FS_RIGHT_ROTATION;
}
static const fs_step_t *animation_step(uint8_t state, uint32_t elapsed) {
    const fs_animation_t *a = &fs_animations[state];
    if (a->loop) elapsed %= a->total_ms;
    for (uint8_t i = 0; i < a->count; ++i) {
        if (elapsed < a->steps[i].duration_ms) return &a->steps[i];
        elapsed -= a->steps[i].duration_ms;
    }
    return &a->steps[a->count - 1];
}
static void render_layers(const fs_step_t *step) {
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    uint8_t mods = get_mods() | get_weak_mods();
#ifndef NO_ACTION_ONESHOT
    mods |= get_oneshot_mods() | get_oneshot_locked_mods();
#endif
    if (drawn_layer == layer && drawn_mods == mods && drawn_lamp == step->lamp_on && drawn_alert == step->alert) return;
    drawn_layer = layer; drawn_mods = mods; drawn_lamp = step->lamp_on; drawn_alert = step->alert;
    uint8_t icons = ((mods & MOD_MASK_GUI) ? 1 : 0) | ((mods & MOD_MASK_SHIFT) ? 2 : 0) |
                    ((mods & MOD_MASK_CTRL) ? 4 : 0) | ((mods & MOD_MASK_ALT) ? 8 : 0);
    static uint8_t framebuffer[512];
    fs_render_left(framebuffer, layer, icons, step->lamp_on, step->alert);
    oled_set_cursor(0, 0);
    oled_write_raw((const char *)framebuffer, sizeof(framebuffer));
}
static void render_diagnostic(void) {
    if (diagnostic_drawn) return;
    diagnostic_drawn = true; oled_clear();
    oled_set_cursor(0, 2); oled_write_P(is_keyboard_left() ? PSTR("LEFT") : PSTR("RIGHT"), false);
    oled_set_cursor(0, 4); oled_write_P(PSTR("32x"), false);
    oled_set_cursor(0, 5); oled_write_P(PSTR("128"), false);
    oled_set_cursor(0, 8); oled_write_P(is_keyboard_master() ? PSTR("USB") : PSTR("LINK"), false);
    // Asymmetric corner bars reveal rotation and clipping.
    for (uint8_t y = 12; y < 16; ++y) for (uint8_t x = 3; x < 10; ++x) oled_write_pixel(x, y, true);
    for (uint8_t y = 112; y < 120; ++y) for (uint8_t x = 25; x < 29; ++x) oled_write_pixel(x, y, true);
    // Text clears its cell background, so draw the clipping-test border last.
    for (uint8_t x = 0; x < 32; ++x) { oled_write_pixel(x, 0, true); oled_write_pixel(x, 127, true); }
    for (uint8_t y = 0; y < 128; ++y) { oled_write_pixel(0, y, true); oled_write_pixel(31, y, true); }
}
bool oled_task_user(void) {
    uint32_t elapsed, now = sync_timer_read32(), activity = last_input_activity_time();
    uint8_t current_state;
    bool music;
    ATOMIC_BLOCK_RESTORESTATE {
        elapsed = fs_update(&station, now, activity);
        current_state = station.state;
        music = fs_music_valid(&station, now);
    }
    bool sleeping = current_state == FS_SLEEP;
    if (sleeping && elapsed >= fs_animations[FS_SLEEP].total_ms) {
        oled_off(); drawn_frame = 255; drawn_layer = 255; diagnostic_drawn = false;
        return false;
    }
    oled_on();
#ifdef FS_DIAGNOSTIC_MODE
    (void)render_layers; (void)animation_step;
    render_diagnostic();
#else
    (void)render_diagnostic;
    const fs_step_t *step = animation_step(current_state, elapsed);
    if (is_keyboard_left()) render_layers(step);
    else {
        uint8_t index = step->frame;
        static uint8_t meter;
        meter = current_state == FS_TYPING ? fs_meter_level(get_current_wpm(), meter) : 0;
        bool show_music = music && (current_state == FS_TYPING || current_state == FS_MUSIC);
        uint8_t music_phase = show_music ? 1 + (now / 1200u) % 2u : 0;
        if (drawn_frame != index || drawn_meter != meter || drawn_music_phase != music_phase) {
            drawn_frame = index; drawn_meter = meter; drawn_music_phase = music_phase;
            static uint8_t framebuffer[512];
            memcpy(framebuffer, fs_pixels[index], sizeof(framebuffer));
            fs_overlay_right(framebuffer, current_state == FS_TYPING, meter, show_music, music_phase == 2);
            oled_set_cursor(0, 0); oled_write_raw((const char *)framebuffer, sizeof(framebuffer));
        }

    }
#endif
    return false;
}
