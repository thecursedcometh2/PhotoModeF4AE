"""Collect MSVC source diagnostics after a failed build, without hiding its failure.

Uses XMake's compilation database and checks every original plugin translation
unit. This is a diagnostic syntax check, never a substitute for compiling/linking.
"""
import concurrent.futures
import json
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[1]
entries = json.loads((root / 'compile_commands.json').read_text(encoding='utf-8'))


def check(entry):
    directory = pathlib.Path(entry['directory'])
    source = pathlib.Path(entry['file'])
    if not source.is_absolute():
        source = directory / source
    if not source.resolve().is_relative_to(root / 'src') or source.suffix != '.cpp':
        return None
    args = entry['arguments']
    if pathlib.Path(args[0]).stem.lower() != 'cl':
        raise RuntimeError('Diagnostics require MSVC compilation commands')
    # Ignore the PCH cache itself but keep its forced header include. This avoids
    # diagnosing a missing PCH in place of errors in the original source.
    args = [arg for arg in args if not arg.lower().startswith(('/yu', '/fp', '/fd'))
            and arg.lower() not in ('/zi', '/z7')]
    result = subprocess.run(args + ['/Zs'], cwd=directory,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return str(source.relative_to(root)), result.returncode, result.stdout.decode(errors='replace')


with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    results = [result for result in pool.map(check, entries) if result]
for name, status, output in results:
    print(f'::group::{name} — exit {status}')
    print(output)
    print('::endgroup::')
print(f'MSVC syntax diagnostics completed for {len(results)} plugin translation units')
sys.exit(1 if not results or any(status for _, status, _ in results) else 0)
