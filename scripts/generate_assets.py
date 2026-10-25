#!/usr/bin/env python3
"""Pack exact portrait grids into the RP2040 QMK OLED framebuffer."""
from pathlib import Path
import hashlib,json
P=Path(__file__).resolve().parents[1]
(P/'build/results').mkdir(parents=True,exist_ok=True)
d=json.loads((P/'assets/field-music.json').read_text());assert (d['width'],d['height'])==(32,128)
def packed(pixels):
 assert len(pixels)==1024
 rows=[int(pixels[y*8:y*8+8],16) for y in range(128)]
 b=bytes(sum(((rows[page*8+bit]>>(31-x))&1)<<bit for bit in range(8)) for page in range(16) for x in range(32))
 for y in range(128):assert sum(((b[(y//8)*32+x]>>(y%8))&1)<<(31-x) for x in range(32))==rows[y]
 return b
def array(name, b):
 return [f'static const uint8_t {name} = {{']+['    '+', '.join(f'0x{x:02x}' for x in b[i:i+16])+',' for i in range(0,len(b),16)]+['};']
unique=[];animations=[];total=0
for state,g in d['gestures'].items():
 sequence=[]
 for f in g['frames']:
  b=packed(f['pixels']);total+=1
  if b not in unique:unique.append(b)
  sequence.append((unique.index(b),f['duration_ms'],int(f['lamp_on']),int(f['alert'])))
 animations.append((state,sequence,g['loop']))
out=['// Generated exact Young fern portrait art; GPL-2.0-or-later.','#pragma once','#include "station_logic.h"',
 'typedef struct { uint16_t duration_ms; uint8_t frame; bool lamp_on; bool alert; } fs_step_t;',
 'typedef struct { const fs_step_t *steps; uint8_t count; uint16_t total_ms; bool loop; } fs_animation_t;']
out.append(f'static const uint8_t fs_pixels[{len(unique)}][512] = {{')
for b in unique:
 out.append('    {');out.extend('        '+', '.join(f'0x{x:02x}' for x in b[i:i+16])+',' for i in range(0,512,16));out.append('    },')
out.append('};')
for state,seq,loop in animations:
 out.append(f'static const fs_step_t fs_{state}_steps[] = {{'+', '.join('{'+f'{ms}, {idx}, {lamp}, {alert}'+'}' for idx,ms,lamp,alert in seq)+'};')
out.append('static const fs_animation_t fs_animations[FS_STATE_COUNT] = {')
for state,seq,loop in animations:
 out.append('    {'+f'fs_{state}_steps, {len(seq)}, {sum(f[1] for f in seq)}, '+str(loop).lower()+'},')
out.append('};')
(P/'firmware/station_assets.h').write_text('\n'.join(out)+'\n')
left=['// Exact Young fern dashboard assets, memory-mapped const data on RP2040.','#pragma once','#include <stdint.h>']
left+=array('fs_left_base[512]',packed(d['left_base']))
left+=array('fs_font[768]',d['font'][:128*6])
left.append('static const uint8_t fs_mod_symbols[4][7] = {')
for name in ['cmd','shift','ctrl','alt']:left.append('    {'+', '.join(str(int(s,2)) for s in d['symbols'][name])+'},')
left.append('};')
(P/'firmware/station_left_assets.h').write_text('\n'.join(left)+'\n')
# Binary fixtures keep the native rendering test independent of C table syntax.
fixtures=P/'tests/fixtures';fixtures.mkdir(exist_ok=True)
records=[]
for i,case in enumerate(d['left_reference_cases']):
 filename=f'left-{i:02}.bin';(fixtures/filename).write_bytes(packed(case['pixels']))
 records.append({k:v for k,v in case.items() if k!='pixels'}|{'file':filename})
(fixtures/'cases.json').write_text(json.dumps(records,indent=2)+'\n')
report={'authored_frames':total,'unique_right_bitmaps':len(unique),'right_bitmap_bytes':len(unique)*512,'dashboard_reference_cases':len(records),'pixel_roundtrip':'pass','art_sha256':hashlib.sha256((P/'assets/field-music.json').read_bytes()).hexdigest()}
(P/'build/results/art-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
