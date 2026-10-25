"""Exercise the actual QMK key hook against tap-hold, modifiers and key classes."""
from pathlib import Path
import subprocess
P=Path(__file__).resolve().parents[1];W=P/'build/native'
s=(P/'firmware/station.c').read_text();adapter=s[s.index('static uint8_t typing_mods('):s.index('void raw_hid_receive_kb(')]
pre=r'''
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "keycodes.h"
#include "station_logic.h"
#define MOD_LCTL 1
#define MOD_LALT 4
#define MOD_LGUI 8
#define MOD_MASK_CTRL 0x11
#define MOD_MASK_ALT 0x44
#define MOD_MASK_GUI 0x88
#define QK_MODS_GET_MODS(kc) (((kc)>>8)&0x1f)
#define ATOMIC_BLOCK_RESTORESTATE for(int once=1;once;once=0)
typedef struct {struct {bool pressed;} event;struct {uint8_t count;} tap;} keyrecord_t;
static fs_logic_t station;
static uint8_t normal,weak,oneshot,locked;
static uint32_t now=1000;
static unsigned count;
static bool master=true;
static uint8_t get_mods(void){return normal;}
static uint8_t get_weak_mods(void){return weak;}
static uint8_t get_oneshot_mods(void){return oneshot;}
static uint8_t get_oneshot_locked_mods(void){return locked;}
static bool is_keyboard_master(void){return master;}
static uint32_t sync_timer_read32(void){return now;}
bool wpm_keycode_user(uint16_t keycode);
static void update_wpm(uint16_t kc){if(wpm_keycode_user(kc))count++;}
'''
post=r'''
static void press(uint16_t kc,uint8_t taps,bool expected_typing,unsigned expected_count){
 station.typing_seen=false;count=0;keyrecord_t record={.event={true},.tap={taps}};
 update_wpm(kc);assert(process_record_user(kc,&record));
 assert(station.typing_seen==expected_typing);assert(count==expected_count);
 record.event.pressed=false;station.typing_seen=false;assert(process_record_user(kc,&record));assert(!station.typing_seen);
}
int main(void){
 press(KC_A,0,true,1);press(KC_SPC,0,true,1);press(KC_P4,0,true,1);
 press(KC_LEFT,0,false,0);press(KC_LSFT,0,false,0);press(MS_LEFT,0,false,0);
 press(KC_BSPC,0,true,0);press(KC_ENT,0,true,0);
 press(QK_LAYER_TAP|(2<<8)|KC_Z,0,false,0);press(QK_LAYER_TAP|(2<<8)|KC_Z,1,true,1);
 press(QK_MOD_TAP|(2<<8)|KC_A,0,false,0);press(QK_MOD_TAP|(2<<8)|KC_A,1,true,1);
 press(0x0200|KC_1,0,true,1);press(0x0800|KC_A,0,false,0);press(0x1800|KC_A,0,false,0);
 normal=2;press(KC_A,0,true,1);normal=8;press(KC_A,0,false,0);normal=0;
 weak=1;press(KC_A,0,false,0);weak=0;oneshot=4;press(KC_A,0,false,0);oneshot=0;
 locked=8;press(KC_A,0,false,0);locked=0;
 puts("PASS: actual QMK hooks preserve key processing, continuous character input, resolved layer/mod taps, shifted symbols, numpad input; ignore holds/navigation/modifiers/shortcuts; no dual-role WPM double counting.");
}
'''
f=W/'typing-adapter-test.c';f.write_text(pre+adapter+post)
exe=W/'test-typing-adapter'
subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(P/'firmware'),'-I',str(P/'tests/include'),str(f),str(P/'firmware/station_logic.c'),'-o',str(exe)],check=True)
out=subprocess.check_output([str(exe)],text=True);(P/'build/results/typing-adapter-tests.txt').write_text(out);print(out,end='')
