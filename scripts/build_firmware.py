#!/usr/bin/env python3
"""Build one physical half. No flash/reset/device operations."""
import argparse, hashlib, json, os, shutil, subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('side', choices=['left', 'right'])
args = parser.parse_args()
lock = json.loads((ROOT / 'upstream.json').read_text())
source = Path(os.environ.get('QMK_HOME', ROOT / '.cache/vial-qmk')).resolve()
if not source.is_dir():
    raise SystemExit('Run python3 scripts/prepare.py first.')
head = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
if head != lock['commit']:
    raise SystemExit('Wrong upstream revision; refusing to build.')
for entry in lock['submodules']:
    expected, relative = entry.strip().split()[:2]
    actual = subprocess.check_output(['git', '-C', str(source / relative), 'rev-parse', 'HEAD'], text=True).strip()
    if expected != actual:
        raise SystemExit(f'Wrong submodule revision: {relative}')
name = 'frog_station_' + args.side
destination = source / 'keyboards/lily58/rev1/keymaps' / name
destination.mkdir(parents=True, exist_ok=True)
for item in (ROOT / 'firmware').iterdir():
    if item.is_file():
        shutil.copy2(item, destination / item.name)
with (destination / 'config.h').open('a') as f:
    f.write('\n#define INIT_EE_HANDS_' + args.side.upper() + '\n')
environment = dict(os.environ, QMK_HOME=str(source))
subprocess.run(['make', f"{lock['target']}:{name}", f"CONVERT_TO={lock['converter']}", '-j4'], cwd=source, env=environment, check=True)
out = ROOT / 'build'; out.mkdir(exist_ok=True)
binary = source / f"lily58_rev1_{name}_{lock['converter']}.uf2"
target = out / f'frog-station-{args.side}.uf2'
shutil.copy2(binary, target)
(out / f'frog-station-{args.side}.sha256').write_text(hashlib.sha256(target.read_bytes()).hexdigest() + '  ' + target.name + '\n')
print(target)
