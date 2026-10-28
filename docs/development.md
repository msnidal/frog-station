# Build and test

Python 3.10+, a C compiler, and Pillow are enough for the native tests and demos:

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r requirements.txt
sh scripts/test.sh
python3 scripts/render_demo.py
```

The tests cover sustained typing, tap-hold handling, state priority, lease expiry,
sleep/wake, split timestamps, framebuffer boundaries and the helper wire protocol.
Display fixtures compare the left renderer against 72 approved framebuffers.
Environment tests check rain occlusion, open gaps, text readability and all poses.

## Firmware toolchain

Use a working QMK ARM toolchain. The checked build used Arm GNU 13.2.Rel1 and the
pinned Vial tree. [QMK's setup guide](https://docs.qmk.fm/newbs_getting_started)
covers system packages. `prepare.py` fetches the exact source commit and its
submodules into `.cache/vial-qmk`. Install that tree's Python dependencies in a
virtual environment:

```sh
python3 scripts/prepare.py
python3 -m pip install -r .cache/vial-qmk/requirements.txt
python3 scripts/build_firmware.py left
python3 scripts/build_firmware.py right
```

An existing checkout at the exact pin can be used with `QMK_HOME=/path/to/vial-qmk`.
The builder creates the generated keymap directories `frog_station_left` and
`frog_station_right` there. Put `arm-none-eabi-gcc` on PATH. Firmware output is
written to `build/`; no device access is performed.

## Mac app

On macOS with Xcode Command Line Tools:

```sh
sh macos/build.sh
"build/Frog Station.app/Contents/MacOS/FrogStation" --self-test
open "build/Frog Station.app"
```

The app and its transport are universal ARM64/x86_64 binaries and are locally
ad-hoc signed. The self-test sends no Spotify events and does not access USB.
`--probe` performs a read-only keyboard identity/capability check.

Public distribution still needs Developer ID signing/notarization and an install
check on another Mac. Keep signing credentials outside the repository.

## Artwork

`assets/field-music.json` is the source: 32-bit rows for every frame, durations,
loop behavior, font, modifier symbols, fern pixels and opaque object spans.
To regenerate C tables and fixtures:

```sh
python3 scripts/generate_assets.py
sh scripts/test.sh
python3 scripts/render_demo.py
```

The demo calls the compiled display and environment renderer with the same source
grids. GIFs sample every 50 ms; firmware requests a frame every 33 ms. The demo
shows framebuffer pixels, not a camera recording of OLED brightness or flicker.

Rain and fern sway use QMK's split-synchronized clock and continue across gesture
changes. They do not update input activity or delay screen sleep. Both renderers
start from a fresh scene each frame, so moving pixels leave no trails.

Optional definitions in `firmware/config.h`:

| Definition | Default | Range |
| --- | --- | --- |
| `FS_RAIN_SPEED` | `48` | 20–90 pixels/second |
| `FS_RAIN_DROPS` | `7` | 0–12 per screen; 0 disables rain |
| `FS_FERN_SWAY` | `1` | 0 disables fern motion |

Rain stays behind the frog, station and mushroom. Occlusion follows their
silhouettes; gaps between supports remain open. The music note has a small
reading margin, and the top 28 rows are reserved for status and modifiers.
