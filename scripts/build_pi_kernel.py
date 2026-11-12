#!/usr/bin/env python3
"""Build the single course Pi 4 kernel, then both exact-ABI course modules.

Run natively on a 64-bit Pi, or pass --cross-compile=aarch64-linux-gnu- on Linux.
No installation, root access or modification of the running kernel occurs here.
"""
import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASELINE = json.loads((ROOT / 'kernel-baseline.json').read_text())


def run(arguments, **kwargs):
    print('+', ' '.join(map(str, arguments)), flush=True)
    return subprocess.run(list(map(str, arguments)), check=True, **kwargs)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def settings(path):
    result = {}
    for line in path.read_text().splitlines():
        if line.startswith('CONFIG_') and '=' in line:
            key, value = line.split('=', 1)
            result[key] = value
        elif line.startswith('# CONFIG_') and line.endswith(' is not set'):
            result[line[2:-11]] = 'n'
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--cross-compile', default='')
    args = parser.parse_args()
    if platform.system() != 'Linux':
        parser.error('Build on Linux: a 64-bit Pi, or the documented Linux CI cross-build.')
    if not args.cross_compile and platform.machine() not in ('aarch64', 'arm64'):
        parser.error('Native builds require 64-bit Arm; otherwise supply --cross-compile.')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    build = ROOT / BASELINE['build_directory']
    source = build / 'source'
    output = build / 'output'
    build.mkdir(parents=True, exist_ok=True)
    output.mkdir(exist_ok=True)
    if not source.exists():
        run(['git', 'init', source])
        run(['git', '-C', source, 'remote', 'add', 'origin', BASELINE['source_url']])
    if not (source / 'Makefile').exists():
        run(['git', '-C', source, 'fetch', '--depth=1', 'origin', BASELINE['source_commit']])
        run(['git', '-C', source, 'checkout', '--detach', 'FETCH_HEAD'])
    actual_commit = subprocess.check_output(['git', '-C', source, 'rev-parse', 'HEAD'], text=True).strip()
    if actual_commit != BASELINE['source_commit']:
        raise SystemExit('Existing source is not the pinned commit; use a separate clean checkout.')
    if subprocess.check_output(['git', '-C', source, 'status', '--porcelain'], text=True).strip():
        raise SystemExit('Pinned kernel source contains changes; refusing an ambiguous build.')
    package = build / 'official-headers.deb'
    if not package.exists():
        with urllib.request.urlopen(BASELINE['config_package_url']) as response, package.open('wb') as target:
            shutil.copyfileobj(response, target)
    if digest(package) != BASELINE['config_package_sha256']:
        raise SystemExit('Official configuration package SHA-256 mismatch.')
    extracted = build / 'official-config'
    run(['dpkg-deb', '-x', package, extracted])
    original_config = extracted / BASELINE['config_package_path']
    shutil.copy2(original_config, output / '.config')
    environment = os.environ.copy()
    environment.update(ARCH='arm64', CROSS_COMPILE=args.cross_compile, LOCALVERSION='',
                       KBUILD_BUILD_USER='hardware-from-software', KBUILD_BUILD_HOST='course',
                       KBUILD_BUILD_TIMESTAMP='2026-11-12 00:00:00 UTC')
    make = ['make', '-C', source, f'O={output}']
    run([source / 'scripts/config', '--file', output / '.config', '--enable', 'XDP_SOCKETS',
         '--set-str', 'LOCALVERSION', BASELINE['localversion']])
    run([*make, 'olddefconfig'], env=environment)
    original = settings(original_config)
    configured = settings(output / '.config')
    differences = {key: {'before': original.get(key, 'absent'), 'after': configured.get(key, 'absent')}
                   for key in sorted(original.keys() | configured.keys()) if original.get(key) != configured.get(key)}
    (build / 'config-diff.json').write_text(json.dumps(differences, indent=2) + '\n')
    # Compiler capability symbols are recalculated by Kconfig. Human review sees every
    # difference in config-diff.json; preserve the course's actual functional features.
    required = {'CONFIG_XDP_SOCKETS': 'y', 'CONFIG_LOCALVERSION': '"-hfs1-v8"',
                'CONFIG_MODVERSIONS': 'y', 'CONFIG_BCMGENET': 'y', 'CONFIG_BPF_SYSCALL': 'y',
                'CONFIG_GPIO_CDEV': 'y', 'CONFIG_NET_NS': 'y', 'CONFIG_VETH': 'm',
                'CONFIG_EXT4_FS': 'y', 'CONFIG_MMC_BLOCK': 'y', 'CONFIG_DEVTMPFS': 'y'}
    for option, value in required.items():
        if configured.get(option) != value:
            raise SystemExit(f'Required {option}={value} missing.')
    release = subprocess.check_output([*map(str, make), '-s', 'kernelrelease'], env=environment, text=True).strip()
    if release != BASELINE['release']:
        raise SystemExit(f'Unexpected kernel release: {release}')
    run([*make, f'-j{args.jobs}', 'Image', 'modules', 'dtbs'], env=environment)
    if not (output / 'Module.symvers').stat().st_size:
        raise SystemExit('Full build did not produce Module.symvers.')
    run([*make, f'M={ROOT / "examples/kernel"}', 'W=1', 'modules'], env=environment)
    run(['dtc', '-@', '-I', 'dts', '-O', 'dtb', '-o', build / 'hardware-button.dtbo',
         ROOT / 'examples/kernel/hardware-button-overlay.dts'])
    image = output / 'arch/arm64/boot/Image'
    dtb = output / 'arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb'
    for module in ('hello', 'mybutton'):
        vermagic = subprocess.check_output(['modinfo', '-F', 'vermagic', ROOT / f'examples/kernel/{module}.ko'], text=True)
        if vermagic.split()[0] != release:
            raise SystemExit(f'{module} has mismatched vermagic.')
    report = {'status': 'full kernel and both external modules compiled', 'source_commit': actual_commit,
              'release': release, 'cross_compile': args.cross_compile, 'build_machine': platform.machine(),
              'config_sha256': digest(output / '.config'), 'image_sha256': digest(image),
              'dtb_sha256': digest(dtb), 'module_symvers_sha256': digest(output / 'Module.symvers'),
              'external_modules': {name: digest(ROOT / f'examples/kernel/{name}.ko') for name in ('hello', 'mybutton')},
              'compiler': subprocess.check_output([args.cross_compile + 'gcc', '--version'], text=True).splitlines()[0],
              'physical_runtime': 'not exercised'}
    (build / 'build-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
