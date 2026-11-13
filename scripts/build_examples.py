#!/usr/bin/env python3
import argparse, json, os, platform, shutil, subprocess, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPS = ROOT / '.deps'


def run(args, **kw):
    print('+', ' '.join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), check=True, cwd=ROOT, **kw)


def main():
    p = argparse.ArgumentParser();
    p.add_argument('--pico', action='store_true');
    p.add_argument('--host', action='store_true');
    p.add_argument('--linux', action='store_true');
    a = p.parse_args()
    if not any([a.pico, a.host, a.linux]): a.pico = a.host = True
    build = ROOT / '.build';
    build.mkdir(exist_ok=True)
    if a.pico:
        env = os.environ.copy()
        local = DEPS / 'arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin'
        env['PATH'] = os.pathsep.join([str(DEPS / 'venv/bin'), str(local), env.get('PATH', '')])
        env['PICO_SDK_PATH'] = str(DEPS / 'pico-sdk')
        cmake = shutil.which('cmake', path=env['PATH'])
        if not cmake: raise SystemExit('Run python3 scripts/setup_deps.py first')
        run([cmake, '-S', 'examples/pico', '-B', '.build/pico', '-G', 'Ninja', '-DPICO_BOARD=pico2',
             '-DPICO_PLATFORM=rp2350-arm-s', '-DCMAKE_BUILD_TYPE=Debug', f'-Dpicotool_DIR={DEPS}/picotool/picotool'],
            env=env)
        run([cmake, '--build', '.build/pico', '--parallel', '4'], env=env)
    if a.host:
        compiler = shutil.which('clang++') or shutil.which('g++')
        for name, flags in [('protocol_test', []), ('protocol_sanitized', ['-g', '-O1', '-fsanitize=address,undefined',
                                                                           '-fno-omit-frame-pointer'])]:
            run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', *flags,
                 'examples/common/protocol_test.cpp', '-o', build / name])
            run([build / name], timeout=15)
        for name in ['hello', 'serial_client', 'button_client', 'capstone']:
            run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', f'examples/linux/{name}.cpp',
                 '-o', build / name])
    if a.linux:
        if platform.system() != 'Linux': raise SystemExit(
            '--linux requires Linux. Cross-compilation evidence is recorded separately.')
        for path in sorted((ROOT / 'examples/linux').glob('*.cpp')):
            run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', path, '-o', build / path.stem])


if __name__ == '__main__': main()
