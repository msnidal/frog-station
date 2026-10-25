// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
static const uint8_t station_identity[12]={6,0,0,0,0x7e,0xfd,0xfc,0x5b,0x7d,0x39,0x48,6};
static inline bool station_identity_ok(const uint8_t r[32]) { return !memcmp(r,station_identity,12); }
static inline bool station_status_ok(const uint8_t r[32],uint8_t op) {
    const uint8_t prefix[4]={0xf1,'F','S',1};
    return !memcmp(r,prefix,4)&&r[4]==op&&r[5]==0&&r[8]==32&&r[9]==128;
}
static inline bool station_music_capable(const uint8_t r[32]) {
    return station_status_ok(r,0)&&r[10]>=2&&r[11]==0&&(r[12]&1);
}
static inline void station_music_packet(uint8_t p[32],bool playing) {
    memset(p,0,32);p[0]=0xf1;p[1]='F';p[2]='S';p[3]=1;p[4]=2;p[5]=playing;
    if(playing){p[8]=0x40;p[9]=0x1f;} // Eight seconds; two-second heartbeats.
}
