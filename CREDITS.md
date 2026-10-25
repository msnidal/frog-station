# Credits

- [QMK](https://github.com/qmk/qmk_firmware), [Vial](https://github.com/vial-kb/vial-qmk), and [KeyboardHoarders' Vial fork](https://github.com/KeyboardHoarders/vial-qmk). The exact revision and submodules are recorded in `upstream.json`.
- [Lily58](https://github.com/kata0510/Lily58), by Naoki Katahira. The example keymap is adapted from the pinned vendor `keyboards/lily58/rev1/keymaps/vial/keymap.c`. Its copyright notice is retained. Layer indices were reordered to match NAV / NUM, and the original OLED code was replaced.
- The first 128 glyphs of `keyboards/lily58/lib/glcdfont_lily.c` supply the display font. These bytes match the pinned upstream font exactly. `tests/include/keycodes.h` is an unchanged copy of QMK's keycode definitions for the native adapter tests.

Code and artwork were developed with AI assistance and iterated on the physical
keyboard. The demo uses the same pixel grids, animation timings, and display
renderer as the firmware.

## License

This project's combined distribution is GPL-3.0-or-later. Existing
GPL-2.0-or-later notices remain valid under their later-version option. Original
upstream notices are retained; dependencies keep their own licenses. The ARM
firmware uses ChibiOS, which is why the combined firmware distribution uses
GPLv3. The original GPLv2 text is retained in `licenses/` as well.

The frog and scenery source grids are in `assets/field-music.json` under the same
project license, to the extent rights apply. No claim is made that these were
hand-drawn without AI assistance.
