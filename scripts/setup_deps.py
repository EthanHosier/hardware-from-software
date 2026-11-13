#!/usr/bin/env python3
"""Install checkout-local, pinned Pico build tools. macOS Apple Silicon baseline."""
import argparse, hashlib, os, platform, shutil, subprocess, sys, tarfile, urllib.request, zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPS = ROOT / '.deps'
SDK_COMMIT = '079c6f39023649b154152db30f1d781e884879bc'
ARM = 'arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi'


def run(args): subprocess.run([str(x) for x in args], check=True, cwd=ROOT)


def download(url, path, sha):
    if not path.exists():
        print('Downloading', path.name, flush=True)
        tmp = path.with_suffix(path.suffix + '.part')
        req = urllib.request.Request(url, headers={'User-Agent': 'HardwareFromSoftware/1.0'})
        with urllib.request.urlopen(req, timeout=120) as src, tmp.open('wb') as out: shutil.copyfileobj(src, out)
        tmp.rename(path)
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != sha: raise RuntimeError(f'SHA256 mismatch for {path.name}: {actual}')


def main():
    if (platform.system(), platform.machine()) != ('Darwin', 'arm64'):
        raise SystemExit(
            'This pinned installer targets Apple Silicon macOS. On Linux install cmake/ninja/Arm embedded GCC and set PICO_SDK_PATH to SDK2.3.1; see Appendix B.')
    DEPS.mkdir(exist_ok=True)
    venv = DEPS / 'venv'
    if not (venv / 'bin/cmake').exists():
        run([sys.executable, '-m', 'venv', venv])
        run([venv / 'bin/python', '-m', 'pip', 'install', 'cmake==3.31.6', 'ninja==1.11.1.4'])
    sdk = DEPS / 'pico-sdk'
    if not sdk.exists(): run(
        ['git', 'clone', '--depth', '1', '--branch', '2.3.1', 'https://github.com/raspberrypi/pico-sdk.git', sdk])
    actual = subprocess.check_output(['git', '-C', str(sdk), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != SDK_COMMIT: raise RuntimeError('Unexpected SDK checkout; use SDK2.3.1 pinned commit')
    run(['git', '-C', sdk, 'submodule', 'update', '--init', '--depth', '1', 'lib/tinyusb'])
    archive = DEPS / (ARM + '.tar.xz')
    if not (DEPS / ARM / 'bin/arm-none-eabi-gcc').exists():
        download('https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/' + archive.name, archive,
                 'c7c78ffab9bebfce91d99d3c24da6bf4b81c01e16cf551eb2ff9f25b9e0a3818')
        with tarfile.open(archive) as t: t.extractall(DEPS)
    for name, asset, sha in [
        ('openocd', 'openocd-0.12.0%2Bdev-mac.zip', '412caeab08a4613d3fd425fc1fa3fa84aa95ff05bca008f13eacee118e05035d'),
        ('picotool', 'picotool-2.3.1-mac.zip', '7bb3cea5d9f1b000fc758cf4b54774fda5c16a9899d342ef6b0c30138988793b')]:
        path = DEPS / (name + '.zip')
        download('https://github.com/raspberrypi/pico-sdk-tools/releases/download/v2.3.1-0/' + asset, path, sha)
        if not (DEPS / name).exists():
            with zipfile.ZipFile(path) as z: z.extractall(DEPS / name)
    # zip extraction through Python doesn't preserve Unix executable bits.
    for path in [DEPS / 'openocd/openocd', DEPS / 'picotool/picotool/picotool']:
        path.chmod(0o755)
    print('SDK2.3.1, ArmGNU14.2.Rel1, CMake3.31.6, Ninja1.11.1.4; local tools ready.')
    print('Next: python3 scripts/build_examples.py --pico')


if __name__ == '__main__': main()
