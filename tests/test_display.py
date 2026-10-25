#!/usr/bin/env python3
"""Compare compiled firmware dashboard renderer with approved prototype pixels."""
import ctypes,json,subprocess
from pathlib import Path
P=Path(__file__).resolve().parents[1];W=P/'build/native';W.mkdir(parents=True,exist_ok=True)
lib=W/'station-display.dylib'
subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror',('-dynamiclib' if __import__('sys').platform=='darwin' else '-shared'),'-fPIC',str(P/'firmware/station_display.c'),'-o',str(lib)],check=True)
f=ctypes.CDLL(str(lib)).fs_render_left
f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint8,ctypes.c_uint8,ctypes.c_bool,ctypes.c_bool]
cases=json.loads((P/'tests/fixtures/cases.json').read_text())
for case in cases:
 # Canary bytes catch accidental writes beyond the physical OLED buffer.
 storage=(ctypes.c_uint8*544)(*([0xA5]*544));buf=ctypes.cast(ctypes.byref(storage,16),ctypes.POINTER(ctypes.c_uint8))
 f(buf,case['layer'],case['mask'],case['lamp_on'],case['alert'])
 assert bytes(storage[:16])==bytes([0xA5]*16) and bytes(storage[-16:])==bytes([0xA5]*16),case
 assert bytes(storage[16:528])==(P/'tests/fixtures'/case['file']).read_bytes(),case
# Exercise all remaining modifier combinations and guard the layer fallback.
for layer in range(256):
 for modifiers in range(16):
  storage=(ctypes.c_uint8*544)(*([0xA5]*544));buf=ctypes.cast(ctypes.byref(storage,16),ctypes.POINTER(ctypes.c_uint8))
  f(buf,layer,modifiers,True,False)
  assert bytes(storage[:16])==bytes([0xA5]*16) and bytes(storage[-16:])==bytes([0xA5]*16)
result={'dashboard_reference_matches':len(cases),'all_modifier_masks_and_layer_fallback_bounds':'pass'}
(P/'build/results/dashboard-tests.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
