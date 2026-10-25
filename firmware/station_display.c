// SPDX-License-Identifier: GPL-2.0-or-later
#include "station_display.h"
#include "station_left_assets.h"
#include <string.h>

static void pixel(uint8_t b[512], uint8_t x, uint8_t y, bool on) {
    uint16_t at = (y / 8) * 32 + x;
    uint8_t bit = 1u << (y % 8);
    if (on) b[at] |= bit;
    else b[at] &= (uint8_t)~bit;
}
static void character(uint8_t b[512], char c, uint8_t x, uint8_t y) {
    for (uint8_t xx = 0; xx < 5; ++xx)
        for (uint8_t yy = 0; yy < 7; ++yy)
            pixel(b, x + xx, y + yy, (fs_font[(uint8_t)c * 6 + xx] >> yy) & 1);
}
void fs_render_left(uint8_t out[512], uint8_t layer, uint8_t modifiers, bool lamp_on, bool alert) {
    memcpy(out, fs_left_base, 512);
    static const char *const names[] = {"BASE", "NAV", "NUM", "L3", "L4", "L5"};
    const char *name = layer < 6 ? names[layer] : "L?";
    uint8_t length = strlen(name), x = (32 - (length * 6 - 1)) / 2;
    for (uint8_t i = 0; i < length; ++i) character(out, name[i], x + i * 6, 7);
    character(out, layer < 10 ? '0' + layer : '?', 14, 82);
    for (uint8_t m = 0; m < 4; ++m) {
        if (!(modifiers & (1u << m))) continue;
        for (uint8_t y = 0; y < 7; ++y)
            for (uint8_t x = 0; x < 7; ++x)
                pixel(out, m * 8 + x, 20 + y, (fs_mod_symbols[m][y] >> (6 - x)) & 1);
    }
    for (uint8_t y = 95; y < 97; ++y)
        for (uint8_t x = 23; x < 25; ++x) pixel(out, x, y, lamp_on);
    if (alert) {
        for (uint8_t y = 90; y <= 92; ++y) pixel(out, 23, y, true);
        pixel(out, 23, 94, true);
    }
}

void fs_overlay_right(uint8_t out[512], bool typing, uint8_t meter, bool music, bool lamp_off) {
    if (typing) {
        if (meter > 5) meter = 5;
        for (uint8_t i = 0; i < 5; ++i)
            for (uint8_t x = 4 + i * 5; x < 7 + i * 5; ++x)
                for (uint8_t y = 20; y <= 22; ++y) pixel(out, x, y, y == 22 || i < meter);
    }
    if (music) {
        for (uint8_t y = 48; y < 55; ++y) pixel(out, 29, y, true);
        for (uint8_t x = 29; x < 32; ++x) for (uint8_t y = 48; y < 50; ++y) pixel(out, x, y, true);
        for (uint8_t x = 26; x < 29; ++x) for (uint8_t y = 54; y < 56; ++y) pixel(out, x, y, true);
        pixel(out, 28, 53, true);
        if (lamp_off) for (uint8_t x = 26; x < 28; ++x) for (uint8_t y = 61; y < 63; ++y) pixel(out, x, y, false);
    }
}
