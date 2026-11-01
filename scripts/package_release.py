"""Package compiled artifacts; never fetch or redistribute Nexus assets."""
import argparse
import pathlib
import shutil
import zipfile
import hashlib
import json
import subprocess


def package(dll, pdb, output):
    root = pathlib.Path(__file__).resolve().parents[1]
    output.mkdir(parents=True, exist_ok=True)
    for source in (dll, pdb):
        if not source.is_file() or source.stat().st_size == 0:
            raise RuntimeError(f'Missing or empty compiled artifact: {source}')
        shutil.copy2(source, output / source.name)
    with zipfile.ZipFile(output / 'PhotoModeF4-1.11.240.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
        archive.write(dll, 'Data/F4SE/Plugins/' + dll.name)
        for name in ('README.md', 'DEPENDENCIES.md', 'TESTING.md', 'LICENSE.txt', 'THIRD_PARTY_NOTICES.md'):
            archive.write(root / name, name)
        for source in sorted((root / 'licenses').rglob('*')):
            if source.is_file():
                archive.write(source, source.relative_to(root).as_posix())
        # Upstream checkout contains no runtime assets. Preserve any reviewed assets
        # later supplied in Data; fonts must remain an independently installed mod.
        for source in sorted((root / 'Data').rglob('*')):
            if source.is_file():
                if 'ImGuiFonts' in source.parts:
                    raise RuntimeError('External ImGui Fonts must not be bundled')
                archive.write(source, source.relative_to(root).as_posix())


def source_bundle(output):
    root = pathlib.Path(__file__).resolve().parents[1]
    # Git enumerates tracked source only, including recursively pinned submodules.
    # Never copy .git, ignored files, local credentials, or installed font assets.
    tracked = subprocess.check_output(
        ['git', 'ls-files', '--recurse-submodules', '-z'], cwd=root).split(b'\0')
    temporary = output / 'PhotoModeF4-corresponding-source.zip.partial'
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
        for item in tracked:
            if not item:
                continue
            relative = pathlib.Path(item.decode('utf-8'))
            source = root / relative
            # Upstream ImGui ships optional sample fonts. They are not build inputs
            # and must not be confused with the separately installed runtime fonts.
            if relative.suffix.lower() in ('.ttf', '.otf', '.ttc', '.woff', '.woff2'):
                continue
            if 'ImGuiFonts' in relative.parts or not source.is_file():
                raise RuntimeError(f'Unexpected source entry: {relative}')
            archive.write(source, 'PhotoModeF4/' + relative.as_posix())
        revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root)
        submodules = subprocess.check_output(['git', 'submodule', 'status', '--recursive'], cwd=root)
        archive.writestr('SOURCE_REVISION.txt', revision + submodules)
        lock = root / 'xmake-requires.lock'
        if lock.is_file() and b'xmake-requires.lock' not in tracked:
            archive.write(lock, 'PhotoModeF4/xmake-requires.lock')
        manifest = json.loads((root / 'scripts/dependency_sources.json').read_text())
        for dependency in manifest:
            errors = []
            for url in dependency['urls']:
                try:
                    data = subprocess.check_output([
                        'curl', '--fail', '--location', '--silent', '--show-error',
                        '--max-time', '60', url])
                    if hashlib.sha256(data).hexdigest() != dependency['sha256']:
                        raise RuntimeError('archive SHA256 mismatch')
                    archive.writestr('dependency-sources/' + dependency['name'] + '-' + url.rsplit('/', 1)[-1], data)
                    break
                except Exception as error:
                    errors.append(f'{url}: {error}')
            else:
                raise RuntimeError('Cannot obtain verified corresponding source: ' + '; '.join(errors))
    temporary.replace(output / 'PhotoModeF4-corresponding-source.zip')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--dll', required=True, type=pathlib.Path)
    parser.add_argument('--pdb', required=True, type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    parser.add_argument('--source-bundle', action='store_true')
    args = parser.parse_args()
    package(args.dll, args.pdb, args.output)

    if args.source_bundle:
        source_bundle(args.output)
