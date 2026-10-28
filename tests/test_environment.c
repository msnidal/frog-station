// SPDX-License-Identifier: GPL-2.0-or-later
#include "station_display.h"
#include "station_assets.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static bool lit(const uint8_t *b, unsigned x, unsigned y) {
    return (b[y / 8 * 32 + x] >> (y % 8)) & 1u;
}
static void check_frame(const uint8_t base[512], uint8_t panel, uint32_t now, bool music) {
    uint8_t storage[544], *out = storage + 16;
    memset(storage, 0xa5, sizeof(storage));
    memcpy(out, base, 512);
    fs_render_environment(out, panel, now, music);
    for (unsigned i = 0; i < 16; ++i) assert(storage[i] == 0xa5 && storage[528+i] == 0xa5);
    for (unsigned y = 0; y < 128; ++y) for (unsigned x = 0; x < 32; ++x) {
        // Only fern pixels may move; rain cannot erase any UI or foreground.
        if ((y < 28 || y >= 74) && lit(base, x, y)) assert(lit(out, x, y));
    }
    // The opaque equipment case must retain its text, lamp and dark interior.
    if (panel == 0) for (unsigned y = 76; y <= 97; ++y)
        for (unsigned x = 7; x <= 26; ++x) assert(lit(out, x, y) == lit(base, x, y));
    if (panel == 1) {
        // The four-letter status word and its inner spaces stay readable.
        for (unsigned y = 5; y <= 15; ++y) for (unsigned x = 5; x <= 26; ++x)
            assert(lit(out, x, y) == lit(base, x, y));
        // The mushroom stem is opaque; the antenna must survive music mode.
        for (unsigned y = 100; y <= 108; ++y) for (unsigned x = 14; x <= 18; ++x)
            assert(lit(out, x, y) == lit(base, x, y));
        assert(lit(out, 26, 62) == lit(base, 26, 62));
        if (music) {
            const uint8_t note[][2] = {{29,48}, {31,49}, {26,55}, {28,53}, {28,50}, {30,54}};
            for (unsigned p = 0; p < sizeof(note) / sizeof(note[0]); ++p)
                assert(lit(out, note[p][0], note[p][1]) == lit(base, note[p][0], note[p][1]));
        }
    }
    uint8_t again[512];
    memcpy(again, base, 512);
    fs_render_environment(again, panel, now, music);
    assert(memcmp(again, out, 512) == 0);
}
int main(void) {
    uint8_t base[512], out[512];
    // Exposed paths remain wet; the fern-covered gaps now stay dry.
    const uint8_t open[2][2][2] = {{{27,52}, {30,106}}, {{29,61}, {30,106}}};
    const uint8_t dry[2][2][2] = {{{10,64}, {16,105}}, {{24,69}, {8,62}}};
    bool reached[2][2] = {{false,false},{false,false}};
    bool sky[2] = {false,false}, gutter[2] = {false,false}, unused_ui[2] = {false,false};
    for (uint8_t panel = 0; panel < 2; ++panel) {
        if (panel == 0) fs_render_left(base, 0, 0, true, false);
        else memcpy(base, fs_pixels[fs_idle_steps[0].frame], 512);
        for (unsigned p = 0; p < 2; ++p) assert(!lit(base, open[panel][p][0], open[panel][p][1]));
        for (uint32_t now = 0; now < 60000; now += 33) {
            memcpy(out, base, 512);
            fs_render_environment(out, panel, now, false);
            for (unsigned p = 0; p < 2; ++p) {
                reached[panel][p] |= lit(out, open[panel][p][0], open[panel][p][1]);
                assert(lit(out, dry[panel][p][0], dry[panel][p][1]) == lit(base, dry[panel][p][0], dry[panel][p][1]));
            }
            sky[panel] |= lit(out, 12, 0);
            gutter[panel] |= lit(out, 30, 10);
            unused_ui[panel] |= lit(out, 12, 22);
            // Dark face pixels are not holes through which rain can pass.
            if (panel == 1) {
                const uint8_t face[][2] = {{13,67}, {11,69}, {17,69}, {14,74}};
                for (unsigned p = 0; p < sizeof(face) / sizeof(face[0]); ++p)
                    assert(lit(out, face[p][0], face[p][1]) == lit(base, face[p][0], face[p][1]));
            }
            check_frame(base, panel, now, false);
        }
        assert(reached[panel][0] && reached[panel][1]);
        assert(sky[panel] && gutter[panel] && unused_ui[panel]);
    }
    for (uint8_t layer = 0; layer < 6; ++layer) for (uint8_t mods = 0; mods < 16; ++mods) {
        fs_render_left(base, layer, mods, true, true);
        check_frame(base, 0, 4800, false);
        const uint8_t lengths[] = {4,3,3,2,2,2};
        unsigned start = (32 - (lengths[layer] * 6 - 1)) / 2;
        for (uint32_t now = 0; now < 12000; now += 131) {
            memcpy(out, base, 512);
            fs_render_environment(out, 0, now, false);
            for (unsigned y = 7; y <= 13; ++y) for (unsigned x = start; x < start + lengths[layer] * 6 - 1; ++x)
                assert(lit(out, x, y) == lit(base, x, y));
            for (unsigned slot = 0; slot < 4; ++slot) if (mods & (1u << slot))
                for (unsigned y = 19; y <= 27; ++y) for (unsigned x = slot * 8; x < slot * 8 + 7; ++x)
                    assert(lit(out, x, y) == lit(base, x, y));
        }
    }
    // The young fern catches this drop, then sways away from column 21.
    // It must not resume its fall beneath the now-open edge of the canopy.
    fs_render_left(base, 0, 0, true, false);
    const uint32_t intercepted[][2] = {{11650,62}, {11683,64}, {11716,65}, {11749,67}, {11782,69}};
    for (unsigned i = 0; i < sizeof(intercepted) / sizeof(intercepted[0]); ++i) {
        memcpy(out, base, 512);
        fs_render_environment(out, 0, intercepted[i][0], false);
        assert(!lit(out, 21, intercepted[i][1]));
    }
    // Check every authored pose, including non-looping task and sleep gestures.
    for (unsigned i = 0; i < sizeof(fs_pixels) / sizeof(fs_pixels[0]); ++i) {
        memcpy(base, fs_pixels[i], 512);
        fs_overlay_right(base, true, 5, true);
        for (uint32_t now = 0; now < 60000; now += 97) {
            check_frame(base, 1, now, true);
            memcpy(out, base, 512);
            fs_render_environment(out, 1, now, true);
            for (unsigned y = 19; y <= 23; ++y) for (unsigned x = 3; x <= 27; ++x)
                assert(lit(out, x, y) == lit(base, x, y));
        }
        check_frame(base, 1, UINT32_MAX, true);
        check_frame(base, 1, UINT32_MAX - 500, true);
    }
    puts("PASS: rain enters from the top, shelters below ferns, preserves active readouts and exposed paths, stays within bounds, and is deterministic across all poses and clock wrap.");
}
