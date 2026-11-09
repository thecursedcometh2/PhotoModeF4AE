"""Copy MCM's English keys into a separate fallback file."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGLISH = ROOT / 'Data/Interface/Translations/PhotoMode_EN.txt'
FALLBACK = ROOT / 'Data/Interface/Translations/PhotoMode_MCMFallback_EN.txt'


def display_keys(value):
    if isinstance(value, str):
        if value.startswith('$'):
            yield value
    elif isinstance(value, dict):
        for item in value.values():
            yield from display_keys(item)
    elif isinstance(value, list):
        for item in value:
            yield from display_keys(item)


def expected_fallback():
    entries = dict(line.split('\t', 1) for line in ENGLISH.read_bytes().decode('utf-16').splitlines() if line)
    config = json.loads((ROOT / 'Data/MCM/Config/PhotoMode/config.json').read_text())
    return ('\r\n'.join(key + '\t' + entries[key] for key in sorted(set(display_keys(config)))) + '\r\n').encode('utf-16')


if __name__ == '__main__':
    FALLBACK.write_bytes(expected_fallback())
