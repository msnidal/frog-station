// SPDX-License-Identifier: GPL-2.0-or-later
#include "station_logic.h"
#include <string.h>

static uint16_t get16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t get32(const uint8_t *p) { return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16); }
static void put16(uint8_t *p, uint16_t n) { p[0] = n; p[1] = n >> 8; }
static void put32(uint8_t *p, uint32_t n) { put16(p, n); put16(p + 2, n >> 16); }
static bool host_state(uint8_t state) { return state == FS_IDLE || (state >= FS_WORKING && state <= FS_FAILED); }

bool fs_host_valid(const fs_host_t *host, uint32_t now) {
    return host->lease_ms != 0 && (uint32_t)(now - host->received_at) < host->lease_ms;
}
bool fs_apply_host(fs_logic_t *s, uint8_t state, uint16_t generation, uint16_t lease_ms, uint32_t received_at, uint32_t now) {
    if (!host_state(state) || (state != FS_IDLE && (lease_ms < 1000 || lease_ms > FS_HOST_MAX_LEASE_MS))) return false;
    if (!fs_host_valid(&s->host, now) || s->host.state != state || s->host.generation != generation) s->restart_host = true;
    s->host = (fs_host_t){.state = state, .generation = generation, .lease_ms = state == FS_IDLE ? 0 : lease_ms, .received_at = received_at};
    return true;
}
void fs_note_typing(fs_logic_t *s, uint32_t now) {
    s->typed_at = now;
    s->typing_seen = true;
}
bool fs_music_valid(const fs_logic_t *s, uint32_t now) {
    return s->music_lease_ms && (uint32_t)(now - s->music_received_at) < s->music_lease_ms;
}
bool fs_apply_music(fs_logic_t *s, bool playing, uint16_t lease_ms, uint32_t received_at, uint32_t now) {
    (void)now;
    if (playing && (lease_ms < 1000 || lease_ms > FS_HOST_MAX_LEASE_MS)) return false;
    s->music_received_at = received_at;
    s->music_lease_ms = playing ? lease_ms : 0;
    return true;
}
uint8_t fs_meter_level(uint8_t wpm, uint8_t previous) {
    if (!wpm) return 0;
    uint8_t level = (wpm + 29u) / 30u;
    if (level > 5) level = 5;
    // A small dead band avoids flicker around the four band boundaries.
    if (previous >= 1 && previous <= 5) {
        if (level > previous && wpm <= previous * 30u + 3u) return previous;
        if (level < previous && wpm >= (previous - 1u) * 30u - 3u) return previous;
    }
    return level;
}
uint32_t fs_update(fs_logic_t *s, uint32_t now, uint32_t activity) {
    if (!s->initialized) {
        s->initialized = true;
        s->state_since = now;
    }
    s->last_activity = activity;
    uint8_t next;
    if ((uint32_t)(now - activity) >= FS_SLEEP_AFTER_MS) next = FS_SLEEP;
    else if (fs_host_valid(&s->host, now)) next = s->host.state;
    else if (s->typing_seen && (uint32_t)(now - s->typed_at) < FS_TYPING_MS) next = FS_TYPING;
    else if (fs_music_valid(s, now)) next = FS_MUSIC;
    else next = FS_IDLE;
    if (next == FS_SLEEP) s->state_since = activity + FS_SLEEP_AFTER_MS;
    else if (next != s->state || (fs_host_valid(&s->host, now) && s->restart_host)) s->state_since = now;
    s->state = next;
    s->restart_host = false;
    return now - s->state_since;
}
void fs_pack_host(const fs_logic_t *s, uint8_t out[FS_WIRE_SIZE]) {
    out[0] = 2; out[1] = s->host.state;
    put16(out + 2, s->host.generation); put32(out + 4, s->host.received_at); put16(out + 8, s->host.lease_ms);
    out[10] = s->music_lease_ms != 0;
    put32(out + 11, s->music_received_at); put16(out + 15, s->music_lease_ms);
    out[17] = s->typing_seen; put32(out + 18, s->typed_at);
}
bool fs_unpack_host(fs_logic_t *s, const uint8_t *in, size_t length, uint32_t now) {
    if (length != FS_WIRE_SIZE || in[0] != 2 || in[10] > 1 || in[17] > 1) return false;
    fs_logic_t candidate = *s;
    if (!fs_apply_host(&candidate, in[1], get16(in + 2), get16(in + 8), get32(in + 4), now) ||
        !fs_apply_music(&candidate, in[10], get16(in + 15), get32(in + 11), now)) return false;
    candidate.typing_seen = in[17]; candidate.typed_at = get32(in + 18);
    *s = candidate;
    return true;
}
bool fs_handle_hid(fs_logic_t *s, uint8_t *data, size_t length, uint32_t now) {
    if (length != 32 || data[0] != 0xF1 || data[1] != 'F' || data[2] != 'S') return false;
    uint8_t op = data[4], error = 0;
    if (data[3] != 1) error = 1;
    else if (op == 1) {
        if (!fs_apply_host(s, data[5], get16(data + 6), get16(data + 8), now, now)) error = 2;
    } else if (op == 2) {
        if (data[5] > 1 || !fs_apply_music(s, data[5] != 0, get16(data + 8), now, now)) error = 2;
    } else if (op != 0) error = 3;
    memset(data, 0, length);
    data[0] = 0xF1; data[1] = 'F'; data[2] = 'S'; data[3] = 1; data[4] = op; data[5] = error;
    data[6] = s->state; data[7] = fs_host_valid(&s->host, now); data[8] = 32; data[9] = 128;
    put16(data + 10, FS_INTERFACE_REVISION);
    data[12] = 1; // Capability bit 0: separate leased music channel.
    data[13] = fs_music_valid(s, now);
    return true;
}
