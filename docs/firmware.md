# Firmware

The checked configuration is KeyboardHoarders' wired Lily58/rev1 with RP2040
controllers using the `rp2040_ce` converter. Each OLED is physically 128×32,
rotated 270° to a 32×128 portrait framebuffer. OLED I²C uses GPIO 2/3; the split
serial link uses GPIO 1. This is not a universal Lily58 image.

The display code was tested on the physical keyboard with USB attached to either
half. The repository's vendor-based example keymap is compiled separately from
the personal layout used during that testing. There are no personal layout
exports or full-flash recovery images in this repository.

## Build

Install a QMK-compatible ARM build environment, then:

```sh
python3 scripts/prepare.py
python3 scripts/build_firmware.py left
python3 scripts/build_firmware.py right
```

The source revision is pinned in `upstream.json`. Outputs go to `build/`.
These commands never flash, reset, or unlock a keyboard.
See [development](development.md) for dependencies.

## Preserve your layout

Save your current layout from Vial before starting. Firmware or storage changes
can reset the dynamic keymap. This example uses six layers: base 0, NAV 1, NUM 2,
ADJ 3, and two spare layers. Its default base layout uses dedicated thumb layer
keys. Import your own compatible `.vil` after flashing; check all layers,
tap-hold settings, macros and dynamic features. Keep the export locally.

## Flash both halves

1. Fully quit Vial. Unplug USB before changing the cable between the halves.
2. Disconnect the halves. Attach USB only to the physical left half.
3. Double-press reset. When `RPI-RP2` appears, copy `frog-station-left.uf2` onto it.
4. Leave the halves disconnected. Move USB to the right half, enter its
   bootloader, and copy `frog-station-right.uf2` onto it.
5. Unplug USB, reconnect the halves, then attach USB normally. Do not reset.
6. Import your saved layout in Vial and verify typing, layers, and settings.
   Check with USB on either half, restoring the export on each host half if needed.

Use the matching image for each physical side; handedness is initialized by the
side-specific build. Both halves need this version of the private split protocol.
Keep your previous compatible firmware and exported layout for rollback.

Check sustained typing, the 1.5-second return to idle, and both screens sleeping
after about 63 seconds. The station should stay physically left and frog right
regardless of the USB side. The optional helper adds Spotify only after these
keyboard checks pass.
