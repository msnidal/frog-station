// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum fs_state { FS_IDLE, FS_TYPING, FS_WORKING, FS_WAITING, FS_DONE, FS_FAILED, FS_SLEEP, FS_MUSIC, FS_STATE_COUNT };
#define FS_SLEEP_AFTER_MS 60000u
#define FS_TYPING_MS 1500u
#define FS_HOST_MAX_LEASE_MS 30000u
#define FS_WIRE_SIZE 22u
#define FS_INTERFACE_REVISION 2u
typedef struct {
    uint32_t received_at;
    uint16_t generation;
    uint16_t lease_ms;
    uint8_t state;
} fs_host_t;
typedef struct {
    fs_host_t host;
    uint32_t state_since;
    uint32_t last_activity;
    uint32_t typed_at;
    uint32_t music_received_at;
    uint16_t music_lease_ms;
    bool typing_seen;
    uint8_t state;
    bool initialized;
    bool restart_host;
} fs_logic_t;

void fs_note_typing(fs_logic_t *s, uint32_t now);
bool fs_music_valid(const fs_logic_t *s, uint32_t now);
bool fs_apply_music(fs_logic_t *s, bool playing, uint16_t lease_ms, uint32_t received_at, uint32_t now);
uint8_t fs_meter_level(uint8_t wpm, uint8_t previous);
bool fs_host_valid(const fs_host_t *host, uint32_t now);
bool fs_apply_host(fs_logic_t *s, uint8_t state, uint16_t generation, uint16_t lease_ms, uint32_t received_at, uint32_t now);
uint32_t fs_update(fs_logic_t *s, uint32_t now, uint32_t activity);
void fs_pack_host(const fs_logic_t *s, uint8_t out[FS_WIRE_SIZE]);
bool fs_unpack_host(fs_logic_t *s, const uint8_t *in, size_t length, uint32_t now);
// Returns false for another protocol; recognized packets receive an in-place ACK.
bool fs_handle_hid(fs_logic_t *s, uint8_t *data, size_t length, uint32_t now);
