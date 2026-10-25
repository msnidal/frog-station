#include "station_logic.h"
#include "link_protocol.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 fs_logic_t keyboard={0};uint8_t p[32]={0xf1,'F','S',1,0};
 assert(fs_handle_hid(&keyboard,p,32,1000));assert(station_music_capable(p));
 station_music_packet(p,true);assert(fs_handle_hid(&keyboard,p,32,1100));assert(station_status_ok(p,2)&&p[13]);
 fs_update(&keyboard,1100,1000);assert(keyboard.state==FS_MUSIC);
 fs_note_typing(&keyboard,1200);fs_update(&keyboard,1200,1200);assert(keyboard.state==FS_TYPING);
 fs_update(&keyboard,2700,1200);assert(keyboard.state==FS_MUSIC);
 station_music_packet(p,false);assert(fs_handle_hid(&keyboard,p,32,3000));assert(station_status_ok(p,2)&&!p[13]);
 fs_update(&keyboard,3000,1200);assert(keyboard.state==FS_IDLE);
 puts("PASS: compiled helper packet builder and firmware parser agree on capability detection, play, typing overlap and pause.");
}
