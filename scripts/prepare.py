#!/usr/bin/env python3
"""Fetch the pinned firmware tree; never access a keyboard."""
import json, os, subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
lock = json.loads((ROOT / 'upstream.json').read_text())
source = Path(os.environ.get('QMK_HOME', ROOT / '.cache/vial-qmk')).resolve()
def git(*args):
    return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()
if not source.exists():
    source.mkdir(parents=True)
    subprocess.run(['git', 'init', str(source)], check=True)
    subprocess.run(['git', '-C', str(source), 'remote', 'add', 'origin', lock['repository']], check=True)
    subprocess.run(['git', '-C', str(source), 'fetch', '--depth=1', 'origin', lock['commit']], check=True)
    subprocess.run(['git', '-C', str(source), 'checkout', '--detach', 'FETCH_HEAD'], check=True)
if git('rev-parse', 'HEAD') != lock['commit']:
    raise SystemExit('The existing checkout is not at the required revision; it was not modified.')
subprocess.run(['git', '-C', str(source), 'submodule', 'update', '--init', '--recursive', '--depth=1'], check=True)
print(source)
