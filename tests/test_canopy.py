"""Compare rain against the visible fronds, including each swaying pose."""
import ctypes
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'build/native'
data = json.loads((ROOT / 'assets/field-music.json').read_text())

def renderer(name, flags):
    path = WORK / (name + '.dylib')
    subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-dynamiclib' if sys.platform == 'darwin' else '-shared', '-fPIC',
                    *flags, str(ROOT / 'firmware/station_display.c'), '-o', str(path)], check=True)
    library = ctypes.CDLL(str(path))
    library.fs_render_environment.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8,
                                              ctypes.c_uint32, ctypes.c_bool]
    return library

wet = ctypes.CDLL(str(WORK / 'station-display.dylib'))
wet.fs_render_environment.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8,
                                     ctypes.c_uint32, ctypes.c_bool]
dry = renderer('station-no-rain', ['-DFS_RAIN_DROPS=0'])
still = renderer('station-no-weather', ['-DFS_RAIN_DROPS=0', '-DFS_FERN_SWAY=0'])

def packed(pixels):
    rows = [int(pixels[y*8:y*8+8], 16) for y in range(128)]
    return bytes(sum(((rows[page*8+bit] >> (31-x)) & 1) << bit for bit in range(8))
                 for page in range(16) for x in range(32))

def render(library, base, panel, now):
    buffer = (ctypes.c_uint8*512).from_buffer_copy(base)
    library.fs_render_environment(buffer, panel, now, False)
    return bytes(buffer)

def lit(buffer, x, y):
    return (buffer[y//8*32+x] >> (y % 8)) & 1

cases = 0
for panel in range(2):
    buffer = (ctypes.c_uint8*512)()
    if panel == 0:
        wet.fs_render_left(buffer, 0, 0, True, False)
        base = bytes(buffer)
    else:
        base = packed(data['gestures']['idle']['frames'][0]['pixels'])
    # Inspect only the visible fern artwork and its one-pixel movement area.
    fern_area = {(x+dx, y) for fern in data['environment']['ferns'] if fern['panel'] == panel
                 for x, y in fern['points'] for dx in (0, 1)}
    outlines = set()
    for now in range(0, 24000, 83):
        scene = render(dry, base, panel, now)
        rain = render(wet, base, panel, now)
        assert render(still, base, panel, now) == base
        roof = [128] * 32
        for x, y in fern_area:
            if lit(scene, x, y):
                roof[x] = min(roof[x], y)
        outlines.add(tuple(roof))
        for x, top in enumerate(roof):
            for y in range(top, 128):
                assert lit(rain, x, y) == lit(scene, x, y), (panel, now, x, y)
        cases += 1
    assert len(outlines) >= 3, 'Shelter must follow the moving fronds.'
print(f'PASS: {cases} canopy frames match the visible leaves; disabled weather preserves the scene.')
