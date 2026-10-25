// SPDX-License-Identifier: GPL-2.0-or-later
#include "station_logic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    fs_logic_t s = {0};
    fs_update(&s, 500, 0); assert(s.state == FS_IDLE); // No false typing at boot.
    fs_note_typing(&s, 1000); fs_update(&s, 1000, 1000);
    assert(s.state == FS_TYPING && s.state_since == 1000);
    // Continuous input must retain TYPE, without replaying its entrance.
    for (uint32_t t = 1100; t <= 30100; t += 100) {
        fs_note_typing(&s, t); fs_update(&s, t, t);
        assert(s.state == FS_TYPING && s.state_since == 1000);
    }
    fs_update(&s, 31599, 30100); assert(s.state == FS_TYPING);
    fs_update(&s, 31600, 30100); assert(s.state == FS_IDLE);
    fs_update(&s, 32000, 32000); assert(s.state == FS_IDLE); // A modifier/navigation event alone.

    assert(fs_apply_music(&s, true, 8000, 33000, 33000));
    fs_update(&s, 33000, 32000); assert(s.state == FS_MUSIC);
    fs_apply_music(&s, true, 8000, 35000, 35000);
    fs_update(&s, 35000, 32000); assert(s.state_since == 33000); // Heartbeat does not restart nod.
    fs_note_typing(&s, 36000); fs_update(&s, 36000, 36000);
    assert(s.state == FS_TYPING && fs_music_valid(&s, 36000));
    fs_update(&s, 37500, 36000); assert(s.state == FS_MUSIC);
    assert(fs_apply_host(&s, FS_WAITING, 7, 2000, 38000, 38000));
    fs_update(&s, 38000, 36000); assert(s.state == FS_WAITING);
    fs_note_typing(&s, 39500); fs_update(&s, 39500, 39500); assert(s.state == FS_WAITING);
    fs_update(&s, 40000, 39500); assert(s.state == FS_TYPING && fs_music_valid(&s, 40000));
    fs_update(&s, 41000, 39500); assert(s.state == FS_MUSIC);
    fs_update(&s, 43000, 39500); assert(s.state == FS_IDLE); // Exact music lease boundary.
    fs_apply_music(&s, true, 8000, 44000, 44000);
    fs_apply_music(&s, false, 0, 44100, 44100); fs_update(&s, 44100, 39500); assert(s.state == FS_IDLE);

    // Neither helper heartbeats nor task status prevent the existing OLED timeout.
    fs_apply_music(&s, true, 8000, 100000, 100000);
    fs_apply_host(&s, FS_WORKING, 8, 30000, 100000, 100000);
    fs_update(&s, 100000, 39500); assert(s.state == FS_SLEEP && s.state_since == 99500);
    fs_update(&s, 100100, 100100); assert(s.state == FS_WORKING);
    fs_apply_host(&s, FS_IDLE, 9, 0, 100101, 100101);
    fs_update(&s, 100101, 100100); assert(s.state == FS_MUSIC); // Shift wakes to music without TYPE.

    fs_note_typing(&s, 101000);
    uint8_t wire[FS_WIRE_SIZE]; fs_pack_host(&s, wire);
    fs_logic_t remote = {0}; assert(fs_unpack_host(&remote, wire, sizeof(wire), 101050));
    fs_update(&remote, 101050, 101000); assert(remote.state == FS_TYPING && fs_music_valid(&remote, 101050));
    assert(fs_unpack_host(&remote, wire, sizeof(wire), 108001));
    fs_update(&remote, 108001, 101000); assert(remote.state == FS_IDLE); // Old RPC renews nothing.
    fs_logic_t before = remote; wire[10] = 2;
    assert(!fs_unpack_host(&remote, wire, sizeof(wire), 108001));assert(!memcmp(&before,&remote,sizeof(remote)));
    assert(!fs_unpack_host(&remote, wire, sizeof(wire)-1, 108001));

    fs_logic_t wrap = {0}; fs_note_typing(&wrap, UINT32_MAX-500);
    fs_apply_music(&wrap, true, 1000, UINT32_MAX-500, UINT32_MAX-500);
    fs_update(&wrap, 200, UINT32_MAX-500); assert(wrap.state == FS_TYPING && fs_music_valid(&wrap,200));
    fs_update(&wrap, 1000, UINT32_MAX-500); assert(wrap.state == FS_IDLE && !fs_music_valid(&wrap,1000));

    uint8_t music[32] = {0xf1,'F','S',1,2,1,0,0,0x40,0x1f};
    fs_logic_t hid = {0}; assert(fs_handle_hid(&hid,music,32,1000));assert(music[5]==0 && music[10]==2 && music[12]==1 && music[13]==1);
    before=hid; uint8_t invalid[32]={0xf1,'F','S',1,2,2,0,0,0x40,0x1f};
    assert(fs_handle_hid(&hid,invalid,32,2000) && invalid[5]==2);assert(!memcmp(&before,&hid,sizeof(hid)));
    uint8_t too_long[32]={0xf1,'F','S',1,2,1,0,0,0xff,0xff};
    assert(fs_handle_hid(&hid,too_long,32,2000) && too_long[5]==2);assert(!memcmp(&before,&hid,sizeof(hid)));
    uint8_t foreign[32]={0xfe,1};assert(!fs_handle_hid(&hid,foreign,32,2000));assert(foreign[0]==0xfe);
    uint8_t version[32]={0xf1,'F','S',2};assert(fs_handle_hid(&hid,version,32,2000)&&version[5]==1);
    uint8_t short_packet[3]={0xf1,'F','S'};assert(!fs_handle_hid(&hid,short_packet,3,2000));
    uint8_t release[32]={0xf1,'F','S',1,2,0};assert(fs_handle_hid(&hid,release,32,2000)&&release[5]==0&&!fs_music_valid(&hid,2000));
    assert(fs_meter_level(0,5)==0 && fs_meter_level(255,0)==5 && fs_meter_level(62,0)==3);
    assert(fs_meter_level(31,1)==1 && fs_meter_level(34,1)==2);
    assert(fs_meter_level(29,2)==2 && fs_meter_level(26,2)==1);
    for (unsigned wpm=0;wpm<256;wpm++)for(unsigned prior=0;prior<6;prior++)assert(fs_meter_level(wpm,prior)<=5);
    puts("PASS: continuous typing, pause expiry, stable animation timing, music/typing coexistence, task priority, sleep/wake, lease expiry, split freshness, atomic rejection, timer wrap, bounded HID and meter hysteresis.");
}
