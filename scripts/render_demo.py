#!/usr/bin/env python3
"""Render firmware pixel grids/timings through the compiled overlay functions."""
import ctypes, json, subprocess, sys
from pathlib import Path
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
work = ROOT / 'build/native'; work.mkdir(parents=True, exist_ok=True)
library = work / 'station-display.dylib'
subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-dynamiclib' if sys.platform == 'darwin' else '-shared', '-fPIC',
                str(ROOT / 'firmware/station_display.c'), '-o', str(library)], check=True)
lib = ctypes.CDLL(str(library))
left = lib.fs_render_left
left.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8, ctypes.c_uint8, ctypes.c_bool, ctypes.c_bool]
overlay = lib.fs_overlay_right
overlay.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_bool, ctypes.c_uint8, ctypes.c_bool, ctypes.c_bool]
data = json.loads((ROOT / 'assets/field-music.json').read_text())
def pack(pixels):
    rows = [int(pixels[y*8:y*8+8], 16) for y in range(128)]
    return bytes(sum(((rows[page*8+bit] >> (31-x)) & 1) << bit for bit in range(8))
                 for page in range(16) for x in range(32))
def image(buffer):
    result = Image.new('RGB', (32, 128), 'black')
    result.putdata([(255,255,255) if buffer[y//8*32+x] >> (y%8) & 1 else (0,0,0)
                    for y in range(128) for x in range(32)])
    return result
def frame(state, elapsed, clock, *, layer=0, mods=0, meter=0, music=False):
    gesture = data['gestures'][state]
    total = sum(f['duration_ms'] for f in gesture['frames'])
    if state == 'sleep' and elapsed >= total:
        return Image.new('RGB',(32,128),'black'), Image.new('RGB',(32,128),'black')
    local = elapsed % total if gesture['loop'] else min(elapsed, total-1)
    chosen = gesture['frames'][-1]
    for item in gesture['frames']:
        if local < item['duration_ms']:
            chosen = item; break
        local -= item['duration_ms']
    a = (ctypes.c_uint8*512)()
    left(a, layer, mods, chosen['lamp_on'], chosen['alert'])
    b = (ctypes.c_uint8*512).from_buffer_copy(pack(chosen['pixels']))
    overlay(b, state == 'typing', meter, music, (clock//1200)%2 == 1)
    return image(a), image(b)
def pair(a,b):
    canvas = Image.new('RGB',(72,128),(17,23,19))
    canvas.paste(a,(0,0)); canvas.paste(b,(40,0))
    return canvas
def save(images, path, scale):
    frames = [x.resize((x.width*scale,x.height*scale), Image.Resampling.NEAREST)
              .quantize(colors=4, dither=Image.Dither.NONE) for x in images]
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=50,
                   loop=0, disposal=2, optimize=False)
docs = ROOT / 'docs'; (docs/'animations').mkdir(exist_ok=True)
frames = []
# A shortened idle excerpt includes its real 150 ms blink; other gestures start at zero.
for state, duration, offset, music in [('idle',3000,4500,False), ('typing',4500,0,False),
                                      ('music',4500,0,True), ('typing',3000,0,True)]:
    for t in range(0,duration,50):
        meter = min(5,1+t//700) if state == 'typing' else 0
        a,b = frame(state,t+offset,len(frames)*50,layer=2 if state=='typing' and t>=2200 else 0,
                    mods=2 if state=='typing' and 800<=t<1400 else 0,meter=meter,music=music)
        frames.append(pair(a,b))
save(frames,docs/'demo.gif',4)
save(frames,docs/'demo-native.gif',1)
frames[0].resize((288,512),Image.Resampling.NEAREST).save(docs/'demo-poster.png')
for state, gesture in data['gestures'].items():
    length = sum(f['duration_ms'] for f in gesture['frames'])
    if not gesture['loop']:length += 1500
    images = [frame(state,t,t,meter=3 if state=='typing' else 0,music=state=='music')[1]
              for t in range(0,length,50)]
    save(images,docs/'animations'/f'{state}.gif',3)
print('Rendered exact-grid demo and eight gesture loops.')
