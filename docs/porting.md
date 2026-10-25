# Other keyboards

Only the hardware in [firmware](firmware.md) has been tested. A 32×128 display
alone does not make another board compatible with the provided configuration.

The reusable parts are `station_logic.c`, `station_display.c`, and the art tables.
`station.c` connects them to QMK's input hooks, synchronized timers, WPM, raw HID,
and split transport. Integrate its hooks with your existing hooks rather than
defining either hook twice. Keep the key processing return values intact.

Review controller conversion, pins, matrix, handedness, display orientation,
storage capacities, and available flash before building for another board.
Change the helper's VID/PID and Vial identity checks for your own device, keeping
the interface revision and capability handshake. The current helper intentionally
refuses mismatched or older firmware.

Layer labels are in `station_display.c`. The four modifier icons represent GUI,
Shift, Control, and Alt. The typing meter is a smoothed five-band estimate, about
30 WPM per band; it measures this keyboard only.

WORK, WAIT, DONE and FAIL are available through the [host protocol](host-protocol.txt).
They require a separate producer. The Spotify app does not automatically infer
editor, build, or AI-agent status.
