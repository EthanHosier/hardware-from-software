#!/usr/bin/env python3
"""Install the locally built single Pi 4 course kernel. Does not reboot.

Only the standard Bookworm Pi OS arm64, direct ext4-on-microSD root layout is
accepted. Stock kernel/DTB/overlays remain untouched; config.txt has an exact
backup that can be restored from another computer without booting Linux.
"""
import json
import os
import platform
import re
import shutil
import subprocess
from pathlib import Path

from build_pi_kernel import BASELINE, ROOT, digest, settings

BEGIN = '# BEGIN hardware-from-software kernel selection\n'
END = '# END hardware-from-software kernel selection\n'


def selected_config(original):
    """Produce firmware configuration while retaining original text in backup."""
    if BEGIN in original or END in original:
        raise ValueError('Course selection already exists without a valid installation record.')
    forbidden = re.compile(r'^\s*(?:include\s+|(?:kernel|device_tree|overlay_prefix|os_prefix|initramfs)\s*(?:=|\s+))')
    for line in original.splitlines():
        if not line.lstrip().startswith('#') and forbidden.match(line):
            raise ValueError('Custom boot selection/include/initramfs found; this installer supports the standard image only.')
    # Put overlay_prefix before any dtoverlay line, so every overlay comes from
    # this kernel's matching directory. Suppress stock automatic-initramfs lines:
    # this direct ext4 SD-root baseline has MMC/ext4 built into the kernel.
    lines = []
    for line in original.splitlines(keepends=True):
        if re.match(r'^\s*auto_initramfs\s*=', line):
            lines.append('# Stock setting disabled by course installer: ' + line)
        else:
            lines.append(line)
    block = (BEGIN + '[pi4]\n'
             'kernel=hfs-kernel/kernel8.img\n'
             'device_tree=hfs-kernel/bcm2711-rpi-4-b.dtb\n'
             'overlay_prefix=hfs-kernel/overlays/\n'
             'auto_initramfs=0\n'
             '[all]\n' + END + '\n')
    return block + ''.join(lines)


def verify_build(build):
    source = build / 'source'
    output = build / 'output'
    report = json.loads((build / 'build-report.json').read_text())
    if report['source_commit'] != BASELINE['source_commit'] or report['release'] != BASELINE['release']:
        raise ValueError('Build report does not match the course baseline.')
    if report['cross_compile'] or report['build_machine'] not in ('aarch64', 'arm64'):
        raise ValueError('Install only a full native Pi build; cross-build host tools are incompatible with the Pi.')
    if subprocess.check_output(['git', '-c', f'safe.directory={source}', '-C', source, 'rev-parse', 'HEAD'], text=True).strip() != BASELINE['source_commit']:
        raise ValueError('Native source tree changed since compilation.')
    if subprocess.check_output(['git', '-c', f'safe.directory={source}', '-C', source,
                                'status', '--porcelain'], text=True).strip():
        raise ValueError('Native source tree has uncommitted changes since compilation.')
    config = settings(output / '.config')
    if config.get('CONFIG_XDP_SOCKETS') != 'y' or config.get('CONFIG_MODVERSIONS') != 'y':
        raise ValueError('Required AF_XDP and symbol versions are missing.')
    for file, key in ((output / '.config', 'config_sha256'),
                      (output / 'Module.symvers', 'module_symvers_sha256'),
                      (output / 'arch/arm64/boot/Image', 'image_sha256'),
                      (output / 'arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb', 'dtb_sha256')):
        if digest(file) != report[key]:
            raise ValueError(f'Build artifact changed: {file}')
    for relative, checksum in report['installed_output_sha256'].items():
        artifact = output / relative
        if not artifact.resolve().is_relative_to(output.resolve()) or digest(artifact) != checksum:
            raise ValueError(f'Compiled module/overlay changed: {relative}')
    actual = (output / 'include/config/kernel.release').read_text().strip()
    if actual != BASELINE['release']:
        raise ValueError('Generated kernel release does not match.')
    return source, output, report


def installed_files_match(destination, modules, output, report):
    """Check an existing installation before claiming an idempotent no-op."""
    if not (modules / 'modules.dep').is_file():
        return False
    for path, checksum in ((destination / 'kernel8.img', report['image_sha256']),
                           (destination / 'bcm2711-rpi-4-b.dtb', report['dtb_sha256'])):
        if not path.is_file() or digest(path) != checksum:
            return False
    for relative, checksum in report['installed_output_sha256'].items():
        compiled = Path(relative)
        if compiled.suffix == '.dtbo' and compiled.parent == Path('arch/arm64/boot/dts/overlays'):
            installed = destination / 'overlays' / compiled.name
            if not installed.is_file() or digest(installed) != checksum:
                return False
        elif compiled.suffix == '.ko':
            # modules_install may compress modules according to the kernel config.
            installed = modules / 'kernel' / compiled
            if not any(Path(str(installed) + suffix).is_file() for suffix in ('', '.xz', '.zst', '.gz')):
                return False
    return (modules / 'build').resolve() == output and (modules / 'source').resolve() == output.parent / 'source'


def main():
    if os.geteuid() != 0 or platform.system() != 'Linux' or platform.machine() != 'aarch64':
        raise SystemExit('Run with sudo on the 64-bit Raspberry Pi 4 after the native full build.')
    model = Path('/proc/device-tree/model').read_bytes().rstrip(b'\0').decode()
    if not model.startswith('Raspberry Pi 4 Model B'):
        raise SystemExit('This installer is limited to Raspberry Pi 4 Model B.')
    root_source = subprocess.check_output(['findmnt', '-n', '-o', 'SOURCE', '/'], text=True).strip()
    root_type = subprocess.check_output(['findmnt', '-n', '-o', 'FSTYPE', '/'], text=True).strip()
    if root_type != 'ext4' or Path(root_source).resolve() != Path('/dev/mmcblk0p2'):
        raise SystemExit('Requires the standard direct ext4 /dev/mmcblk0p2 root; encrypted/USB/other layouts need separate validation.')
    build = ROOT / BASELINE['build_directory']
    try:
        source, output, report = verify_build(build)
        boot = Path('/boot/firmware')
        if not os.path.ismount(boot):
            raise ValueError('/boot/firmware must be the mounted microSD firmware partition.')
        config_path = boot / 'config.txt'
        backup = boot / 'config.txt.hfs-original'
        destination = boot / BASELINE['boot_subdirectory']
        record_path = destination / 'build-report.json'
        modules = Path('/lib/modules') / BASELINE['release']
        if record_path.exists():
            if (json.loads(record_path.read_text()) == report and backup.is_file()
                    and config_path.read_bytes().decode() == selected_config(backup.read_bytes().decode())
                    and installed_files_match(destination, modules, output, report)):
                print('The same course kernel is already installed; no files changed.')
                return
            raise ValueError('Existing course installation differs; refusing to replace it.')
        if backup.exists() or destination.exists() or modules.exists():
            raise ValueError('A backup/destination/modules directory already exists; inspect the prior partial installation before proceeding.')
        original = config_path.read_bytes().decode()
        updated = selected_config(original)
        image = output / 'arch/arm64/boot/Image'
        dtbs = output / 'arch/arm64/boot/dts'
        overlays = list((dtbs / 'overlays').glob('*.dtbo'))
        if not overlays or not (source / 'arch/arm64/boot/dts/overlays/README').is_file():
            raise ValueError('Full matching overlay build is missing.')
        boot_bytes = image.stat().st_size + sum(path.stat().st_size for path in overlays) + 32 * 1024 * 1024
        if shutil.disk_usage(boot).free < boot_bytes:
            raise ValueError('Insufficient free boot-partition space for this separate kernel directory.')
        # Stage modules first; boot selection is the final write. modules_install
        # uses the full native build's generated symbol/version information.
        staging = build / 'install-staging'
        if staging.exists():
            raise ValueError('Prior install-staging exists; inspect it before retrying.')
        install_environment = os.environ.copy()
        install_environment.update(GIT_CONFIG_COUNT='1', GIT_CONFIG_KEY_0='safe.directory',
                                   GIT_CONFIG_VALUE_0=str(source))
        subprocess.run(['make', '-C', source, f'O={output}', 'ARCH=arm64', 'CROSS_COMPILE=', 'LOCALVERSION=',
                        f'INSTALL_MOD_PATH={staging}', 'modules_install'], check=True, env=install_environment)
        staged_modules = staging / 'lib/modules' / BASELINE['release']
        if not staged_modules.is_dir():
            raise ValueError('Matching staged modules missing.')
        shutil.copytree(staged_modules, modules, symlinks=True)
        (modules / 'build').unlink(missing_ok=True)
        (modules / 'build').symlink_to(output, target_is_directory=True)
        (modules / 'source').unlink(missing_ok=True)
        (modules / 'source').symlink_to(source, target_is_directory=True)
        subprocess.run(['depmod', '-a', BASELINE['release']], check=True)
        destination.mkdir()
        shutil.copy2(image, destination / 'kernel8.img')
        shutil.copy2(dtbs / 'broadcom/bcm2711-rpi-4-b.dtb', destination / 'bcm2711-rpi-4-b.dtb')
        (destination / 'overlays').mkdir()
        for overlay in overlays:
            shutil.copy2(overlay, destination / 'overlays' / overlay.name)
        shutil.copy2(source / 'arch/arm64/boot/dts/overlays/README', destination / 'overlays/README')
        shutil.copy2(output / '.config', boot / f'config-{BASELINE["release"]}')
        # Exclusive creation guarantees an earlier original is never overwritten.
        with backup.open('xb') as stream:
            stream.write(original.encode())
        record_path.write_text(json.dumps(report, indent=2) + '\n')
        pending = boot / 'config.txt.hfs-pending'
        pending.write_bytes(updated.encode())
        os.replace(pending, config_path)
        os.sync()
        print(f'Installed {BASELINE["release"]}. No reboot was performed.')
        print(f'Offline rollback: restore {backup.name} to config.txt on the microSD boot volume.')
        print('Keep the source and output trees: the module build link uses them.')
    except (ValueError, FileNotFoundError) as error:
        raise SystemExit(str(error)) from error


if __name__ == '__main__':
    main()
