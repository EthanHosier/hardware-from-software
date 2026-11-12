#!/usr/bin/env python3
"""Build the Linux networking lab and run portable decoder/checksum fixtures.

Pi/Bookworm: sudo apt install build-essential clang llvm pkg-config libxdp-dev
libbpf-dev libelf-dev zlib1g-dev python3 iproute2 ethtool tcpdump
"""
import argparse
import hashlib
import json
import platform
import shlex
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / '.build/network'
SOURCE = ROOT / 'examples/network'


def run(arguments, **kwargs):
    print('+', shlex.join(map(str, arguments)), flush=True)
    return subprocess.run(list(map(str, arguments)), check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--portable-only', action='store_true')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    compiler = shutil.which('g++') if platform.system() == 'Linux' else shutil.which('clang++')
    if not compiler:
        raise SystemExit('A native C++20 compiler is required.')
    flags = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-g', '-O2']
    run([*flags, SOURCE / 'wire_test.cpp', '-o', OUTPUT / 'wire_test'])
    run([OUTPUT / 'wire_test'])
    run([*flags, '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
         SOURCE / 'wire_test.cpp', '-o', OUTPUT / 'wire_test_sanitized'])
    run([OUTPUT / 'wire_test_sanitized'])
    if args.portable_only:
        return
    if platform.system() != 'Linux':
        raise SystemExit('The networking programs require Linux; use --portable-only on macOS.')
    dependencies = subprocess.check_output(['pkg-config', '--cflags', '--libs', 'libxdp', 'libbpf'], text=True)
    for name in ('udp_endpoint', 'packet_rx', 'packet_ring', 'xdp_control', 'xsk_receiver', 'xsk_responder'):
        run([*flags, SOURCE / (name + '.cpp'), '-o', OUTPUT / name,
             *shlex.split(dependencies)])
    clang = shutil.which('clang')
    if not clang:
        raise SystemExit('Install a Linux clang package containing the BPF backend.')
    triple = subprocess.check_output([compiler, '-dumpmachine'], text=True).strip()
    run([clang, '-target', 'bpf', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
         '-I', '/usr/include/' + triple, '-c', SOURCE / 'lab_xdp.bpf.c',
         '-o', OUTPUT / 'lab_xdp.bpf.o'])
    report = {
        'status': 'built; portable fixtures passed; privileged runtime is separate',
        'host_kernel': platform.release(), 'architecture': platform.machine(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'clang': subprocess.check_output([clang, '--version'], text=True).splitlines()[0],
        'libxdp': subprocess.check_output(['pkg-config', '--modversion', 'libxdp'], text=True).strip(),
        'libbpf': subprocess.check_output(['pkg-config', '--modversion', 'libbpf'], text=True).strip(),
        'sources': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in sorted(SOURCE.glob('*')) if path.is_file()},
        'physical_runtime': 'not exercised',
    }
    (OUTPUT / 'build-report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
