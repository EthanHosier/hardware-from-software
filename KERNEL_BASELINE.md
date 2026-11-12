# One Pi 4 kernel throughout the course

The course kernel is **6.12.96-hfs1-v8**, built from Raspberry Pi's
`ae161617d7a4552fa91d03626b8d9f3696d17481` source. Its Makefile identifies Linux
6.12.96. `kernel-baseline.json` is the machine-readable contract.

Use this kernel before the first Linux userspace lesson and keep it for the
existing GPIO, UART, I2C, memory, module, and future networking lessons. Stock
firmware/kernel files remain solely an emergency rollback. No second teaching
profile or later AF_XDP-specific kernel switch is needed.

## Configuration and build

The configuration comes from the SHA-256-pinned official
`linux-headers-6.12.96+rpt-rpi-v8_6.12.96-1+rpt1_arm64.deb`, not an approximate
upstream defconfig. The intentional feature change is `CONFIG_XDP_SOCKETS=y`.
`CONFIG_LOCALVERSION="-hfs1-v8"` gives the result its own release and module
namespace; `CONFIG_LOCALVERSION_AUTO` remains disabled. Kconfig can recalculate
compiler/tool capability symbols; every resulting difference is recorded in
`.build/pi-kernel/config-diff.json` for acceptance review.

On the 64-bit Raspberry Pi OS Bookworm Pi 4, from the repository root:

```sh
sudo apt update
sudo apt install build-essential git bc bison flex libssl-dev libelf-dev dwarves python3 kmod xz-utils device-tree-compiler ca-certificates
python3 scripts/build_pi_kernel.py --jobs 4
sudo python3 scripts/install_pi_kernel.py
```

The build uses `.build/pi-kernel/source` and `.build/pi-kernel/output`. This is a
**full Image, modules and DTBs build**, followed by both existing course modules
with `W=1`, and the GPIO overlay. `Module.symvers` is produced by the full build;
`modules_prepare` alone is not substituted when `CONFIG_MODVERSIONS=y`.
Build-report hashes bind the config, Image, Pi 4 DTB, Module.symvers and both
external modules to the source commit and compiler. This can take significant
space/time; use the available-memory/job count and actual disk space, rather
than an invented universal requirement.

CI performs the same full build in a Debian Bookworm container on an x86 Linux
runner using `aarch64-linux-gnu-`. Separately, an Arm64 Bookworm container builds
and executes the portable/Linux userspace checks. Cross-built x86 host tools
are **not** installed as Pi module headers: the reader's entire build is native.

## Installation and recovery contract

The installer accepts only a native Pi 4 Model B, standard direct ext4
`/dev/mmcblk0p2` root and mounted `/boot/firmware`. It refuses existing custom
kernel/DTB/prefix/initramfs/include directives. Other root or boot arrangements
need separate validation and must not be silently treated as this baseline.
The standard configuration has MMC and ext4 built in; this direct SD-root
installation does not need an initramfs. Encrypted, USB and other layouts are
outside this installer.

It installs the separate kernel and matching Pi 4 DTB under
`/boot/firmware/hfs-kernel/`, all built overlays and their README under
`/boot/firmware/hfs-kernel/overlays/`, and modules under
`/lib/modules/6.12.96-hfs1-v8/`. `/lib/modules/<release>/build` points to the
**native full output tree**, with a source link to the pinned source tree. Keep
both directories; no distro `linux-headers-$(uname -r)` package exists for this
course release. Rebuild course modules with:

```sh
make -C /lib/modules/$(uname -r)/build LOCALVERSION= M="$PWD/examples/kernel" W=1 modules
```

Keep `LOCALVERSION=` explicitly empty in subsequent module builds and release checks. The shallow source checkout is not a release-tag checkout; the kernel can append `+` if this variable is unset. This matches the original full build.

The generated firmware selection is at the start of config.txt so matching
overlays are selected before any existing `dtoverlay` directive:

```ini
# BEGIN hardware-from-software kernel selection
[pi4]
kernel=hfs-kernel/kernel8.img
device_tree=hfs-kernel/bcm2711-rpi-4-b.dtb
overlay_prefix=hfs-kernel/overlays/
auto_initramfs=0
[all]
# END hardware-from-software kernel selection
```

Stock `auto_initramfs` lines are retained as comments so they cannot override
the selection. There is no `os_prefix`: the existing root
`/boot/firmware/cmdline.txt` remains the active file for the UART lesson.
Stock Image/DTB/overlay files are not replaced. Boot configuration is switched
only after module staging and boot-file copying succeed. The exact original
config is saved once as `/boot/firmware/config.txt.hfs-original`; repeated
installation of the same checked build is a no-op, and conflicting or partial
installations are refused for inspection. No automatic reboot occurs.

Before the first reboot, identify the microSD firmware volume on the Mac and
understand offline recovery: shut down/remove Pi power, mount that volume on
the Mac, replace `config.txt` with `config.txt.hfs-original`, eject, and boot.
The backup predates later lesson changes; after recovery those settings need
reapplying deliberately. Restoring the entire backup also restores the original
initramfs policy. Removing only the marker block does not do that.

After reboot, verify the exact `uname -r`, config, matching module tree and
build link, normal SSH/Wi-Fi/Ethernet, and creation of an AF_XDP socket. Enabled
AF_XDP is availability, not traffic redirection: previous socket applications
retain their normal path unless an XDP program is explicitly attached.

## Evidence boundary

A complete build and matching module symbol versions establish compilation and
ABI consistency. Native userspace tests establish only their exercised software
behavior. Neither establishes Pi boot, GPIO, inter-board buses, PHY/DMA behavior,
or loading/unloading the modules on real hardware. Physical Pi boot and all
hardware exercises remain **not exercised** unless a reader actually performs
them and records the result. No connected Pi is modified by CI or this change.

Primary references: [Raspberry Pi kernel workflow](https://www.raspberrypi.com/documentation/computers/linux_kernel.html),
[pinned source Makefile](https://github.com/raspberrypi/linux/blob/ae161617d7a4552fa91d03626b8d9f3696d17481/Makefile),
[Linux external modules](https://docs.kernel.org/6.12/kbuild/modules.html),
[firmware boot configuration](https://www.raspberrypi.com/documentation/computers/config_txt.html).
