// SPDX-License-Identifier: GPL-2.0-or-later
// Include the renderer so the impact and splash primitives can be checked
// without exposing test-only entry points to firmware or the Mac helper.
#include "../firmware/station_display.c"
#include <assert.h>
#include <stdio.h>

int main(void) {
    // A drop crossing two fronds belongs to the first impact only. Later
    // intersections cannot restart its splash beneath the upper canopy.
    assert(fern_hit_y(0, 10, 900, 30000) == 128);
    assert(fern_hit_y(0, 10, 1000, 31000) == 31);
    assert(fern_hit_y(0, 10, 1812, 70000) == 31);

    uint8_t mask[512] = {0}, shelter[32];
    memset(shelter, 128, sizeof(shelter));
    unsigned visible = 0;
    for (unsigned x = 0; x < 32; ++x) for (unsigned y = 29; y <= 73; y += 11)
        for (unsigned ms = 0; ms <= 240; ms += 15) {
            uint8_t storage[544], *out = storage + 16;
            memset(storage, 0xa5, sizeof(storage));
            memset(out, 0, 512);
            fern_splash(out, mask, shelter, x, y, ms * FS_RAIN_SPEED, x);
            unsigned count = 0;
            for (unsigned yy = 0; yy < 128; ++yy) for (unsigned xx = 0; xx < 32; ++xx) if (lit(out, xx, yy)) {
                assert((int)xx >= (int)x - 2 && xx <= x + 2);
                assert(yy >= y - 2 && yy < y);
                ++count;
            }
            assert(count <= 2);
            if (ms >= 180) assert(count == 0);
            visible += count;
            for (unsigned i = 0; i < 16; ++i) assert(storage[i] == 0xa5 && storage[528+i] == 0xa5);
        }
    assert(visible > 0);

    // The splash uses the same occlusion as rain: no marks over a leaf,
    // readout, music glyph, or a column that is already sheltered higher up.
    uint8_t out[512] = {0};
    memset(mask, 0xff, sizeof(mask));
    fern_splash(out, mask, shelter, 15, 31, 80 * FS_RAIN_SPEED, 0);
    for (unsigned i = 0; i < 512; ++i) assert(out[i] == 0);
    memset(mask, 0, sizeof(mask));
    memset(shelter, 28, sizeof(shelter));
    fern_splash(out, mask, shelter, 15, 31, 80 * FS_RAIN_SPEED, 0);
    for (unsigned i = 0; i < 512; ++i) assert(out[i] == 0);
    puts("PASS: first-impact splashes last under 180 ms, use at most two nearby pixels, respect occlusion and stay within bounds.");
}
