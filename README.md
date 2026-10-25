# Frog Station

A frog, a field station, and two small OLEDs on a Lily58.

![Idle, typing, and Spotify playback on the two displays](docs/demo.gif)

Actual 32×128 framebuffers, shown at 4×. [All animations](docs/animations.md).

The left screen shows layers and held modifiers. The frog reacts to typing, with
a five-segment pace meter. An optional Mac menu-bar app adds a quiet Spotify mode.
Typing and the local animations work without the app. Both screens sleep when
the keyboard is idle.

## Install

- [Mac companion](docs/macos.md) · [download preview](https://github.com/msnidal/frog-station/releases)
- [Build and flash firmware](docs/firmware.md)
- [Adapt to another board](docs/porting.md)

Tested hardware: KeyboardHoarders Lily58/rev1, RP2040 controllers using
`rp2040_ce`, two 128×32 OLEDs mounted vertically, Vial. The Mac app requires
macOS 13 or later and supports Apple Silicon and Intel.

This is a private preview. The Mac download is locally signed, not notarized.
The example keymap is based on the vendor layout; export your Vial layout before
flashing and restore it afterwards.

## Source

[Build and test](docs/development.md) · [Host protocol](docs/host-protocol.txt) ·
[Credits](CREDITS.md) · [GPL-3.0-or-later](LICENSE)
