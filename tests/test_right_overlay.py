"""Verify dynamic meter/music pixels and all framebuffer boundaries."""
from pathlib import Path
import ctypes,json
P=Path(__file__).resolve().parents[1];R=P
d=json.loads((P/'assets/field-music.json').read_text())
lib=ctypes.CDLL(str(P/'build/native/station-display.dylib'));f=lib.fs_overlay_right
f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_bool,ctypes.c_uint8,ctypes.c_bool,ctypes.c_bool]
def pack(p):
 rows=[int(p[y*8:y*8+8],16) for y in range(128)]
 return bytes(sum(((rows[page*8+bit]>>(31-x))&1)<<bit for bit in range(8)) for page in range(16) for x in range(32))
def pixel(b,x,y,v):
 index=y//8*32+x;mask=1<<(y%8)
 b[index]=(b[index]|mask) if v else (b[index]&~mask)
cases=0
for gesture in d['gestures'].values():
 for frame in gesture['frames']:
  base=pack(frame['pixels'])
  for typing in [False,True]:
   for meter in [0,1,2,3,4,5,255]:
    for music in [False,True]:
     for off in [False,True]:
      expected=bytearray(base)
      if typing:
       for i in range(5):
        for x in range(4+i*5,7+i*5):
         for y in range(20,23):pixel(expected,x,y,y==22 or i<min(meter,5))
      if music:
       for y in range(48,55):pixel(expected,29,y,True)
       for x in range(29,32):
        for y in range(48,50):pixel(expected,x,y,True)
       for x in range(26,29):
        for y in range(54,56):pixel(expected,x,y,True)
       pixel(expected,28,53,True)
       if off:
        for x in range(26,28):
         for y in range(61,63):pixel(expected,x,y,False)
      storage=(ctypes.c_uint8*544)(*([0xA5]*16+list(base)+[0xA5]*16));buf=ctypes.cast(ctypes.byref(storage,16),ctypes.POINTER(ctypes.c_uint8))
      f(buf,typing,meter,music,off)
      assert bytes(storage[:16])+bytes(storage[-16:])==bytes([0xA5]*32)
      assert bytes(storage[16:528])==expected
      cases+=1
(P/'build/results/right-overlay-tests.json').write_text(json.dumps({'exact_overlay_cases':cases,'bounds':'pass'},indent=2)+'\n');print(cases,'overlay combinations passed, with preserved surrounding art.')
