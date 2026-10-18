#!/usr/bin/env python3
"""Package an existing arm64 build as a relocatable, ad-hoc signed macOS app."""
import argparse
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import tempfile


def run(*args):
    return subprocess.check_output(list(map(str, args)), text=True).strip()


def dependencies(binary):
    return [line.strip().split(' (')[0]
            for line in run('otool', '-L', binary).splitlines()[1:]]


def rpaths(binary):
    lines = run('otool', '-l', binary).splitlines()
    return [lines[i + 2].strip().split(' (')[0].removeprefix('path ')
            for i, line in enumerate(lines) if line.strip() == 'cmd LC_RPATH']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    source = args.source.resolve(strict=True)
    output = args.output.absolute()
    if output.suffix != '.app':
        parser.error('--output must end in .app')
    output.parent.mkdir(parents=True, exist_ok=True)

    def expand(path, loader):
        return Path(path.replace('@loader_path', str(loader.parent))
                    .replace('@executable_path', str(executable.parent)))

    def resolve(dependency, loader):
        if dependency.startswith('@rpath/'):
            suffix = dependency[len('@rpath/'):]
            roots = [expand(p, loader) for p in rpaths(loader)]
            roots += [expand(p, executable) for p in rpaths(executable)]
            for root in roots:
                candidate = root / suffix
                if candidate.exists():
                    return candidate.resolve()
            raise RuntimeError(f'Cannot resolve {dependency} from {loader}')
        return expand(dependency, loader).resolve(strict=True)

    # Resolve original linkage before changing any install names.
    graph = {}
    pending = [executable]
    names = {}
    while pending:
        binary = pending.pop()
        if binary in graph:
            continue
        if run('lipo', '-archs', binary) != 'arm64':
            raise RuntimeError(f'Expected arm64-only binary: {binary}')
        links = {}
        for dependency in dependencies(binary):
            if dependency.startswith(('/System/Library/', '/usr/lib/')):
                continue
            resolved = resolve(dependency, binary)
            if resolved == binary:  # dylib's own install ID
                continue
            if resolved.name in names and names[resolved.name] != resolved:
                raise RuntimeError(f'Duplicate library basename: {resolved.name}')
            names[resolved.name] = resolved
            links[dependency] = resolved
            pending.append(resolved)
        graph[binary] = links

    with tempfile.TemporaryDirectory(prefix='macos-package-', dir=output.parent) as temp:
        app = Path(temp) / output.name
        contents = app / 'Contents'
        macos = contents / 'MacOS'
        resources = contents / 'Resources'
        frameworks = contents / 'Frameworks'
        for directory in (macos, resources, frameworks):
            directory.mkdir(parents=True)
        shutil.copy2(executable, macos / 'FloatingSandbox')
        for directory in ('Data', 'Ships', 'Guides'):
            shutil.copytree(source / directory, resources / directory,
                            ignore=shutil.ignore_patterns('.DS_Store', '._*', '.floatingsandbox_shipdb'))
        shutil.copy2(source / 'Ships/R.M.S. Titanic (With Power).shp2',
                     resources / 'Ships/default_ship.shp2')
        for name in ('license.txt', 'changes.txt', 'README.md'):
            shutil.copy2(source / name, resources / name)
        version_header = (source / 'Sources/Game/GameVersion.h').read_text()
        version = [re.search(r'#define APPLICATION_VERSION_' + part + r'\s+(\d+)',
                             version_header).group(1)
                   for part in ('MAJOR', 'MINOR', 'PATCH', 'BUILD')]
        run('sips', '-s', 'format', 'icns',
            source / 'Sources/FloatingSandbox/Resources/ShipAAA.ico',
            '--out', resources / 'FloatingSandbox.icns')
        notices = resources / 'ThirdPartyLicenses'
        notices.mkdir()
        for original in graph:
            if original == executable:
                continue
            prefix = original.parent.parent
            candidates = [p for p in prefix.iterdir()
                          if p.is_file() and p.name.lower().startswith(('license', 'copying', 'copyright'))]
            sfml_license = prefix / 'share/doc/SFML/license.md'
            if sfml_license.exists():
                candidates.append(sfml_license)
            if candidates:
                destination = notices / original.name
                destination.mkdir(exist_ok=True)
                for license_path in candidates:
                    shutil.copy2(license_path, destination / license_path.name)
        info = {
            'CFBundleName': 'Floating Sandbox',
            'CFBundleDisplayName': 'Floating Sandbox',
            'CFBundleExecutable': 'FloatingSandbox',
            'CFBundleIconFile': 'FloatingSandbox.icns',
            'CFBundleIdentifier': 'org.floatingsandbox.personal',
            'CFBundlePackageType': 'APPL',
            'CFBundleShortVersionString': '.'.join(version[:3]),
            'CFBundleVersion': '.'.join(version),
            'LSMinimumSystemVersion': '26.0',
            'LSArchitecturePriority': ['arm64'],
            'NSHighResolutionCapable': True,
            'NSPrincipalClass': 'NSApplication',
        }
        with (contents / 'Info.plist').open('wb') as stream:
            plistlib.dump(info, stream)
        (contents / 'PkgInfo').write_bytes(b'APPL????')

        for original in graph:
            if original != executable:
                shutil.copy2(original, frameworks / original.name)
        for original, links in graph.items():
            is_executable = original == executable
            binary = macos / 'FloatingSandbox' if is_executable else frameworks / original.name
            binary.chmod(binary.stat().st_mode | 0o200)
            if not is_executable:
                run('install_name_tool', '-id', '@rpath/' + original.name, binary)
            for old, resolved in links.items():
                prefix = '@executable_path/../Frameworks/' if is_executable else '@loader_path/'
                run('install_name_tool', '-change', old, prefix + resolved.name, binary)
            for path in rpaths(binary):
                run('install_name_tool', '-delete_rpath', path, binary)
            run('codesign', '--force', '--sign', '-', binary)
        run('codesign', '--force', '--sign', '-', app)
        run('codesign', '--verify', '--deep', '--strict', app)

        # Reject accidental dependencies on this Mac's build or Homebrew paths.
        for binary in [macos / 'FloatingSandbox', *frameworks.iterdir()]:
            for dependency in dependencies(binary):
                if not dependency.startswith(('/System/Library/', '/usr/lib/',
                                              '@loader_path/', '@executable_path/', '@rpath/')):
                    raise RuntimeError(f'External dependency remains: {dependency}')
        if output.exists():
            if not (output / 'Contents/Info.plist').exists():
                raise RuntimeError(f'Refusing to replace non-bundle: {output}')
            shutil.rmtree(output)
        shutil.move(str(app), output)
    print(f'Packaged {output} with {len(graph) - 1} arm64 libraries')


if __name__ == '__main__':
    main()
