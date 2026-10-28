# Mac companion

The companion reads Spotify's playing/paused/stopped state and sends a small
status packet to the keyboard. It does not read track names, capture audio,
control playback, or monitor Mac keystrokes. Spotify must already be running.

1. Download `Frog-Station-macOS.zip` from [Releases](https://github.com/msnidal/frog-station/releases).
2. Unzip it and move **Frog Station.app** to Applications.
3. Open Spotify, then Frog Station. Click its leaf in the menu bar.
4. Enable **Spotify music mode** and allow its Spotify Automation request.
5. Optionally add Frog Station in **System Settings → General → Login Items & Extensions → Open at Login**.

The preview is ad-hoc signed and has not been notarized. A downloaded
copy may need approval in macOS Privacy & Security. Do not disable Gatekeeper.
Building from source is also supported; see [development](development.md).

The menu shows the keyboard connection and Spotify state. **Authorize Spotify…**
retries the permission request. Music updates roughly every two seconds. If Vial
holds the keyboard interface, quit Vial to let the helper reconnect.

Quitting the app lets the music state expire within eight seconds. The keyboard
continues working normally. The screens still sleep while Spotify is playing;
tap a key to wake them.

Only Spotify is supported. Browser audio and YouTube do not trigger music mode.
The app works on macOS 13+ with Apple Silicon or Intel.
