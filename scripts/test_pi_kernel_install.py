#!/usr/bin/env python3
"""Exercise kernel installation entirely inside temporary fixture directories.

No sudo, real boot/module writes, subprocesses, network or hardware are used.
The real artifact validation, file copies, symlinks and config replacement run.
"""
import contextlib
import hashlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import install_pi_kernel as installer


class KernelInstallTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='hfs-kernel-installer-')
        self.addCleanup(self.temporary.cleanup)
        self.fixture = Path(self.temporary.name).resolve()
        self.project = self.fixture / 'project'
        self.build = self.project / installer.BASELINE['build_directory']
        self.source = self.build / 'source'
        self.output = self.build / 'output'
        self.boot = self.fixture / 'boot'
        self.module_root = self.fixture / 'modules'
        self.release = installer.BASELINE['release']
        self.modules = self.module_root / self.release
        self.destination = self.boot / installer.BASELINE['boot_subdirectory']
        self.model = self.fixture / 'model'
        for directory in (self.source, self.output, self.boot, self.module_root):
            directory.mkdir(parents=True)
        self.model.write_bytes(b'Raspberry Pi 4 Model B Rev 1.5\0')
        self.original = b'# original\r\nauto_initramfs=1\r\n[all]\r\ndtoverlay=vc4-kms-v3d\r\n'
        self.config = self.boot / 'config.txt'
        self.config.write_bytes(self.original)
        self.stock = {
            'kernel8.img': b'stock kernel',
            'bcm2711-rpi-4-b.dtb': b'stock DTB',
            'overlays/disable-bt.dtbo': b'stock overlay',
            'cmdline.txt': b'root=PARTUUID=fixture rootfstype=ext4 rootwait\n',
        }
        for name, contents in self.stock.items():
            self.write(self.boot / name, contents)
        self.files = {
            '.config': (b'CONFIG_XDP_SOCKETS=y\nCONFIG_MODVERSIONS=y\n'),
            'Module.symvers': b'fixture symbol versions\n',
            'arch/arm64/boot/Image': b'fixture custom image',
            'arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb': b'fixture custom DTB',
            'arch/arm64/boot/dts/overlays/disable-bt.dtbo': b'fixture custom overlay',
            'include/config/kernel.release': (self.release + '\n').encode(),
            'drivers/fixture.ko': b'fixture module',
        }
        for name, contents in self.files.items():
            self.write(self.output / name, contents)
        self.write(self.source / 'arch/arm64/boot/dts/overlays/README', b'fixture overlays README')
        self.report = {
            'source_commit': installer.BASELINE['source_commit'],
            'release': self.release,
            'cross_compile': '',
            'build_machine': 'aarch64',
        }
        for name, key in (
            ('.config', 'config_sha256'),
            ('Module.symvers', 'module_symvers_sha256'),
            ('arch/arm64/boot/Image', 'image_sha256'),
            ('arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb', 'dtb_sha256'),
        ):
            self.report[key] = hashlib.sha256(self.files[name]).hexdigest()
        self.report['installed_output_sha256'] = {
            name: hashlib.sha256(contents).hexdigest()
            for name, contents in self.files.items() if name.endswith(('.ko', '.dtbo'))
        }
        self.save_report()
        self.root_source = '/dev/mmcblk0p2'
        self.root_type = 'ext4'
        self.fail_make = False
        self.fail_depmod = False
        self.git_commit = installer.BASELINE['source_commit']
        self.git_status = ''
        self.omit_staged_modules = False
        self.process_calls = []
        self.stack = contextlib.ExitStack()
        self.addCleanup(self.stack.close)
        patches = (
            mock.patch.object(installer, 'ROOT', self.project),
            mock.patch.object(installer, 'Path', side_effect=self.mapped_path),
            mock.patch.object(installer.os, 'geteuid', return_value=0),
            mock.patch.object(installer.platform, 'system', return_value='Linux'),
            mock.patch.object(installer.platform, 'machine', return_value='aarch64'),
            mock.patch.object(installer.os.path, 'ismount', side_effect=lambda path: path == self.boot),
            mock.patch.object(installer.os, 'sync', create=True),
            mock.patch.object(installer.subprocess, 'check_output', side_effect=self.check_output),
            mock.patch.object(installer.subprocess, 'run', side_effect=self.run_process),
        )
        for patch in patches:
            self.stack.enter_context(patch)

    @staticmethod
    def write(path, contents):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(contents)

    def save_report(self):
        (self.build / 'build-report.json').write_text(json.dumps(self.report))

    def mapped_path(self, value):
        mappings = {
            '/boot/firmware': self.boot,
            '/lib/modules': self.module_root,
            '/proc/device-tree/model': self.model,
        }
        return mappings.get(str(value), Path(value))

    def check_output(self, arguments, **kwargs):
        arguments = list(map(str, arguments))
        if arguments[0] == 'findmnt':
            return (self.root_source if 'SOURCE' in arguments else self.root_type) + '\n'
        if arguments[0] == 'git' and arguments[-2:] == ['rev-parse', 'HEAD']:
            return self.git_commit + '\n'
        if arguments[0] == 'git' and arguments[-2:] == ['status', '--porcelain']:
            return self.git_status
        self.fail('Unexpected subprocess: ' + repr(arguments))

    def run_process(self, arguments, **kwargs):
        arguments = list(map(str, arguments))
        self.process_calls.append(arguments)
        if arguments[0] == 'make' and arguments[-1] == 'modules_install':
            if self.fail_make:
                raise installer.subprocess.CalledProcessError(2, arguments)
            if not self.omit_staged_modules:
                staging = Path(next(argument.split('=', 1)[1] for argument in arguments
                                    if argument.startswith('INSTALL_MOD_PATH=')))
                self.write(staging / 'lib/modules' / self.release / 'kernel/drivers/fixture.ko', b'fixture module')
            return None
        if arguments == ['depmod', '-a', self.release]:
            if self.fail_depmod:
                raise installer.subprocess.CalledProcessError(1, arguments)
            self.write(self.modules / 'modules.dep', b'kernel/drivers/fixture.ko:\n')
            return None
        self.fail('Unexpected subprocess: ' + repr(arguments))

    def install(self):
        with contextlib.redirect_stdout(io.StringIO()):
            installer.main()

    def assert_original_untouched(self):
        self.assertEqual(self.config.read_bytes(), self.original)
        for name, contents in self.stock.items():
            self.assertEqual((self.boot / name).read_bytes(), contents)

    def test_successful_install_and_idempotent_rerun(self):
        self.install()
        self.assertEqual((self.boot / 'config.txt.hfs-original').read_bytes(), self.original)
        self.assertEqual(self.config.read_bytes(), installer.selected_config(self.original.decode()).encode())
        for name, contents in self.stock.items():
            self.assertEqual((self.boot / name).read_bytes(), contents)
        self.assertEqual((self.destination / 'kernel8.img').read_bytes(), self.files['arch/arm64/boot/Image'])
        self.assertEqual((self.destination / 'bcm2711-rpi-4-b.dtb').read_bytes(),
                         self.files['arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb'])
        self.assertEqual((self.destination / 'overlays/disable-bt.dtbo').read_bytes(), b'fixture custom overlay')
        self.assertEqual((self.destination / 'overlays/README').read_bytes(), b'fixture overlays README')
        self.assertEqual((self.modules / 'kernel/drivers/fixture.ko').read_bytes(), b'fixture module')
        self.assertTrue((self.modules / 'build').is_symlink())
        self.assertEqual((self.modules / 'build').resolve(), self.output)
        self.assertEqual((self.modules / 'source').resolve(), self.source)
        before = self.config.stat().st_mtime_ns
        calls = len(self.process_calls)
        self.install()
        self.assertEqual(len(self.process_calls), calls)
        self.assertEqual(self.config.stat().st_mtime_ns, before)

    def test_selection_order_initramfs_and_exact_backup_restore(self):
        selected = installer.selected_config(self.original.decode())
        self.assertLess(selected.index('overlay_prefix='), selected.index('dtoverlay='))
        self.assertIn('auto_initramfs=0\n', selected)
        self.assertIn('# Stock setting disabled by course installer: auto_initramfs=1\r\n', selected)
        self.assertNotIn('os_prefix=', selected)
        self.assertNotIn('cmdline=', selected)
        self.install()
        self.config.write_bytes((self.boot / 'config.txt.hfs-original').read_bytes())
        self.assert_original_untouched()

    def test_foreign_boot_selection_is_refused(self):
        for directive in ('kernel=foreign.img', 'device_tree=foreign.dtb', 'os_prefix=other/',
                          'overlay_prefix=other/', 'include other.txt', 'initramfs other followkernel'):
            with self.subTest(directive=directive):
                original = self.original + directive.encode() + b'\n'
                self.config.write_bytes(original)
                with self.assertRaises(SystemExit):
                    self.install()
                self.assertEqual(self.config.read_bytes(), original)
                self.assertFalse(self.destination.exists())
                self.assertFalse(self.modules.exists())
        self.config.write_bytes(self.original)

    def test_partial_installations_and_foreign_modules_are_refused(self):
        for collision in (self.boot / 'config.txt.hfs-original', self.destination, self.modules,
                          self.build / 'install-staging'):
            with self.subTest(collision=collision.name):
                if collision.suffix == '.hfs-original':
                    collision.write_bytes(b'foreign backup')
                else:
                    collision.mkdir()
                with self.assertRaises(SystemExit):
                    self.install()
                self.assert_original_untouched()
                if collision.is_dir():
                    collision.rmdir()
                else:
                    collision.unlink()

    def test_refuses_non_pi_and_nonstandard_root(self):
        self.model.write_bytes(b'Raspberry Pi 5 Model B\0')
        with self.assertRaises(SystemExit):
            self.install()
        self.model.write_bytes(b'Raspberry Pi 4 Model B\0')
        for source, filesystem in (('/dev/sda2', 'ext4'), ('/dev/mmcblk0p2', 'btrfs')):
            self.root_source, self.root_type = source, filesystem
            with self.assertRaises(SystemExit):
                self.install()
        self.assert_original_untouched()
        self.assertFalse(self.modules.exists())

    def test_refuses_wrong_privilege_platform_and_unmounted_boot(self):
        for target, value in ((installer.os, ('geteuid', 1000)),
                              (installer.platform, ('system', 'Darwin')),
                              (installer.platform, ('machine', 'x86_64'))):
            with mock.patch.object(target, value[0], return_value=value[1]):
                with self.assertRaises(SystemExit):
                    self.install()
        with mock.patch.object(installer.os.path, 'ismount', return_value=False):
            with self.assertRaises(SystemExit):
                self.install()
        self.assert_original_untouched()

    def test_staging_failure_never_changes_boot_selection(self):
        self.fail_make = True
        with self.assertRaises(installer.subprocess.CalledProcessError):
            self.install()
        self.assert_original_untouched()
        self.assertFalse((self.boot / 'config.txt.hfs-original').exists())
        self.assertFalse(self.destination.exists())
        self.assertFalse(self.modules.exists())

    def test_missing_staged_modules_never_changes_boot_selection(self):
        self.omit_staged_modules = True
        with self.assertRaises(SystemExit):
            self.install()
        self.assert_original_untouched()
        self.assertFalse(self.destination.exists())

    def test_real_build_validation_rejects_each_changed_artifact(self):
        for name in ('.config', 'Module.symvers', 'arch/arm64/boot/Image',
                     'arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb', 'include/config/kernel.release',
                     'drivers/fixture.ko', 'arch/arm64/boot/dts/overlays/disable-bt.dtbo'):
            path = self.output / name
            original = path.read_bytes()
            with self.subTest(artifact=name):
                path.write_bytes(original + b'changed')
                with self.assertRaises(SystemExit):
                    self.install()
                self.assert_original_untouched()
                self.assertFalse(self.modules.exists())
                path.write_bytes(original)

    def test_depmod_failure_keeps_boot_selection_unchanged(self):
        self.fail_depmod = True
        with self.assertRaises(installer.subprocess.CalledProcessError):
            self.install()
        self.assert_original_untouched()
        self.assertFalse((self.boot / 'config.txt.hfs-original').exists())
        self.assertFalse(self.destination.exists())
        # Modules may have been copied, but a retry must refuse that partial state.
        self.assertTrue(self.modules.exists())
        self.fail_depmod = False
        with self.assertRaises(SystemExit):
            self.install()
        self.assert_original_untouched()

    def test_changed_source_or_installed_configuration_is_refused(self):
        for commit, status in (('foreign', ''), (installer.BASELINE['source_commit'], ' M fixture.c')):
            self.git_commit, self.git_status = commit, status
            with self.assertRaises(SystemExit):
                self.install()
            self.assert_original_untouched()
        self.git_commit = installer.BASELINE['source_commit']
        self.git_status = ''
        self.install()
        changed = self.config.read_bytes() + b'# reader changed configuration\n'
        self.config.write_bytes(changed)
        with self.assertRaises(SystemExit):
            self.install()
        self.assertEqual(self.config.read_bytes(), changed)
        self.assertEqual((self.boot / 'config.txt.hfs-original').read_bytes(), self.original)

    def test_low_boot_space_keeps_existing_state(self):
        with mock.patch.object(installer.shutil, 'disk_usage', return_value=mock.Mock(free=0)):
            with self.assertRaises(SystemExit):
                self.install()
        self.assert_original_untouched()
        self.assertFalse(self.modules.exists())
        self.assertFalse(self.destination.exists())

    def test_corrupted_installed_boot_artifacts_are_refused_on_rerun(self):
        self.install()
        configuration = self.config.read_bytes()
        for name in ('kernel8.img', 'bcm2711-rpi-4-b.dtb', 'overlays/disable-bt.dtbo'):
            path = self.destination / name
            original = path.read_bytes()
            with self.subTest(artifact=name):
                path.write_bytes(b'corrupted installed artifact')
                self.assertFalse(installer.installed_files_match(
                    self.destination, self.modules, self.output, self.report))
                with self.assertRaises(SystemExit):
                    self.install()
                self.assertEqual(self.config.read_bytes(), configuration)
                path.write_bytes(original)

    def test_missing_installed_modules_or_dependency_index_are_refused(self):
        self.install()
        for name in ('kernel/drivers/fixture.ko', 'modules.dep'):
            path = self.modules / name
            original = path.read_bytes()
            with self.subTest(artifact=name):
                path.unlink()
                self.assertFalse(installer.installed_files_match(
                    self.destination, self.modules, self.output, self.report))
                with self.assertRaises(SystemExit):
                    self.install()
                path.write_bytes(original)
        module = self.modules / 'kernel/drivers/fixture.ko'
        compressed = module.with_suffix('.ko.xz')
        module.rename(compressed)
        self.assertTrue(installer.installed_files_match(
            self.destination, self.modules, self.output, self.report))
        self.install()

    def test_cross_build_or_wrong_baseline_is_refused(self):
        for key, value in (('cross_compile', 'aarch64-linux-gnu-'), ('build_machine', 'x86_64'),
                           ('source_commit', 'foreign'), ('release', 'foreign')):
            original = self.report[key]
            with self.subTest(field=key):
                self.report[key] = value
                self.save_report()
                with self.assertRaises(SystemExit):
                    self.install()
                self.assert_original_untouched()
            self.report[key] = original
        self.save_report()


if __name__ == '__main__':
    unittest.main(verbosity=2)
