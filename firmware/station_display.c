// SPDX-License-Identifier: GPL-2.0-or-later
#include "station_display.h"
#include "station_left_assets.h"
#include "station_environment_assets.h"
#include <string.h>

#ifndef FS_RAIN_SPEED
#    define FS_RAIN_SPEED 48
#endif
#ifndef FS_RAIN_DROPS
#    define FS_RAIN_DROPS 7
#endif
#ifndef FS_FERN_SWAY
#    define FS_FERN_SWAY 1
#endif
_Static_assert(FS_RAIN_SPEED >= 20 && FS_RAIN_SPEED <= 90, "Rain speed must be 20..90 pixels/second");
_Static_assert(FS_RAIN_DROPS >= 0 && FS_RAIN_DROPS <= 12, "Rain count must be 0..12 per screen");

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

void fs_overlay_right(uint8_t out[512], bool typing, uint8_t meter, bool music) {
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
    }
}

static bool lit(const uint8_t b[512], uint8_t x, uint8_t y) {
    return (b[(y / 8) * 32 + x] >> (y % 8)) & 1u;
}
static void sway(uint8_t out[512], const fs_fern_t *fern, uint32_t now) {
    uint32_t phase = ((uint64_t)now + fern->phase) % fern->period;
    phase *= 100;
    if (phase < fern->period * 20u || phase >= fern->period * 72u) return;
    uint8_t limit = (phase < fern->period * 31u || phase >= fern->period * 59u) ? fern->tip : fern->full;
    for (uint16_t i = 0; i < fern->count; ++i)
        pixel(out, fern->points[i].x, fern->points[i].y, false);
    for (uint16_t i = 0; i < fern->count; ++i) {
        uint8_t x = fern->points[i].x, y = fern->points[i].y;
        pixel(out, x + (y <= limit), y, true);
        if (y == limit) pixel(out, x, y, true); // Keep the hinge attached.
    }
}
static void rain_mask(uint8_t mask[512], const uint8_t scene[512], uint8_t panel, bool music) {
    memcpy(mask, scene, 512); // Exact lit pixels, with no blanket halo.
    for (uint8_t y = 0; y < 28; ++y)
        for (uint8_t x = 0; x < 32; ++x) pixel(mask, x, y, true);
    for (uint8_t i = 0; i < sizeof(fs_solid_spans) / sizeof(fs_solid_spans[0]); ++i) {
        const fs_span_t *s = &fs_solid_spans[i];
        if (s->panel != panel) continue;
        for (uint8_t x = s->x0; x <= s->x1; ++x) pixel(mask, x, s->y, true);
    }
    if (panel != 1) return;
    // Black eyes and body interiors are solid too. Follow each frame's outline
    // rather than reserving a rectangle around the frog and its moving hand.
    for (uint8_t y = 65; y <= 86; ++y) {
        uint8_t lo = 32, hi = 0;
        for (uint8_t x = 4; x <= 25; ++x) if (lit(scene, x, y)) {
            if (x < lo) lo = x;
            hi = x;
        }
        for (uint8_t x = lo; x <= hi; ++x) pixel(mask, x, y, true);
    }
    if (music) {
        // Reserve a one-pixel reading margin only around the visible note.
        for (uint8_t y = 48; y <= 55; ++y) for (uint8_t x = 26; x < 32; ++x) {
            if (!lit(scene, x, y)) continue;
            for (uint8_t yy = y - 1; yy <= y + 1; ++yy)
                for (uint8_t xx = x - 1; xx <= x + 1 && xx < 32; ++xx) pixel(mask, xx, yy, true);
        }
    }
}
static uint32_t rain_hash(uint32_t n) {
    n = (n ^ (n >> 16)) * 0x45d9f3bu;
    n = (n ^ (n >> 16)) * 0x45d9f3bu;
    return n ^ (n >> 16);
}
static void rain_dot(uint8_t out[512], const uint8_t mask[512], int x, int y) {
    if (x >= 0 && x < 32 && y >= 28 && y < 128 && !lit(mask, x, y)) pixel(out, x, y, true);
}
void fs_render_environment(uint8_t out[512], uint8_t panel, uint32_t now, bool music) {
    if (panel > 1) return;
    if (FS_FERN_SWAY) {
        for (uint8_t i = 0; i < sizeof(fs_ferns) / sizeof(fs_ferns[0]); ++i)
            if (fs_ferns[i].panel == panel) sway(out, &fs_ferns[i], now);
    }
    const uint8_t count = FS_RAIN_DROPS;
    if (!count) return;
    uint8_t mask[512];
    rain_mask(mask, out, panel, music);
    for (uint8_t i = 0; i < count; ++i) {
        uint32_t salt = i * 101u + panel * 1307u;
        // Units are milliseconds * pixels/second. Integer arithmetic preserves
        // sub-millisecond spacing without floating point on the controller.
        uint32_t period = 98000u + (160u + rain_hash(salt + 71u) % 320u) * FS_RAIN_SPEED;
        uint64_t absolute = (uint64_t)now * FS_RAIN_SPEED + (uint64_t)period * (i * 100u + panel * 41u) / (count * 100u);
        uint32_t cycle = absolute / period, local = absolute % period;
        uint32_t seed = rain_hash(salt + cycle * 719u);
        int x = 1 + (i * 11u + panel * 13u + cycle * 7u + seed % 5u) % 30u;
        if (local < 98000u) {
            int head = 28 + local / 1000u;
            for (uint8_t trail = 0; trail < 2 + seed % 2u; ++trail) rain_dot(out, mask, x, head - trail);
        } else if (i == 0 && local < 98000u + 420u * FS_RAIN_SPEED) {
            int radius = local < 98000u + 180u * FS_RAIN_SPEED ? 1 : 2;
            rain_dot(out, mask, x - radius, 126);
            rain_dot(out, mask, x + radius, 126);
        }
    }
}
