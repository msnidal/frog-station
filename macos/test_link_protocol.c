#include "link_protocol.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint8_t id[32]={0};memcpy(id,station_identity,12);assert(station_identity_ok(id));id[4]^=1;assert(!station_identity_ok(id));
 uint8_t old[32]={0xf1,'F','S',1,0,0,0,0,32,128,1,0};assert(!station_music_capable(old));
 old[10]=2;old[12]=1;assert(station_music_capable(old));old[8]=64;assert(!station_music_capable(old));old[8]=32;
 uint8_t p[32];station_music_packet(p,true);assert(p[4]==2&&p[5]==1&&p[8]==0x40&&p[9]==0x1f);
 for(unsigned i=0;i<32;i++)if(i>9||i==6||i==7)assert(p[i]==0);
 station_music_packet(p,false);assert(p[5]==0&&p[8]==0&&p[9]==0);
 puts("PASS: identity, old firmware refusal, display geometry, music capability and bounded eight-second music packets.");
}
