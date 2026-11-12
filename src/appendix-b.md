# B — Build and recovery reference

Use this page to rebuild a known artifact or recover a failed session. Commands labelled **Mac** run locally; commands labelled **Pi** run inside SSH on the Raspberry Pi. **Pico debugger** commands go into GDB. All builds run as your ordinary user, in a user-owned directory.

Compilation needs no connected board. Before standalone Pico programming, shut down the Pi, remove power from **both boards**, then detach **all Pi–Pico wires, including ground**. This prevents a powered transmitter feeding an unpowered board through a signal pin. Keep each board's local LED/button circuit intact. Never join their power rails. Reconnect inter-board wiring only while both boards are off, using the appropriate lesson's circuit.

## 1. Establish the project root

The commands use `~/hardware-from-software`: the directory containing `scripts/`, `examples/` and `EXAMPLES.md`. Substitute your actual location consistently. To obtain a new copy, choose either the [public repository](https://github.com/EthanHosier/hardware-from-software) or the [example archive](downloads/hardware-examples.zip).

**Mac or Pi — working directory `~`; repository route, only if the destination is unused:**

```sh
cd ~
git clone https://github.com/EthanHosier/hardware-from-software.git hardware-from-software
cd ~/hardware-from-software
ls scripts/setup_deps.py scripts/build_examples.py examples/pico/CMakeLists.txt
```

For the archive route, create a new empty `~/hardware-from-software` directory and extract the archive **into it**. The archive has `examples/`, `scripts/` and `EXAMPLES.md` directly, without a wrapper directory. On the **Mac**, with the ZIP in Downloads:

```sh
cd ~
mkdir hardware-from-software
unzip ~/Downloads/hardware-examples.zip -d ~/hardware-from-software
cd ~/hardware-from-software
```

If the directory already exists, inspect and reuse it rather than extracting over edits. The archive is sufficient for the builds below and needs no GitHub account or private configuration.

## 2. Mac firmware setup and build

The supported pinned installer is `scripts/setup_deps.py` (**underscore**, not hyphen). It accepts only native Apple Silicon macOS (`Darwin`, `arm64`). Prerequisites are Apple's command-line developer tools, Git, and Python 3.9+ with `venv`; [1.1](1.1-install-a-reproducible-macos-toolchain.md) explains missing-prerequisite installation. No editor extension is required.

**Mac — project root:**

```sh
cd ~/hardware-from-software
uname -m
xcode-select -p
git --version
python3 --version
python3 scripts/setup_deps.py
python3 scripts/build_examples.py --pico
```

Require `arm64` and success from each step. The installer checks the SDK commit and SHA-256 hashes of the Arm/tool archives. Its pins come from the [SDK 2.3.1 release](https://github.com/raspberrypi/pico-sdk/releases/tag/2.3.1), [Raspberry Pi tools release v2.3.1-0](https://github.com/raspberrypi/pico-sdk-tools/releases/tag/v2.3.1-0), and the URLs/hashes in the downloaded `scripts/setup_deps.py`.

| Installed item | Checkout-relative location / version |
| --- | --- |
| Pico SDK | `.deps/pico-sdk`, 2.3.1, commit `079c6f39023649b154152db30f1d781e884879bc` |
| Arm GCC, G++, GDB, binutils | `.deps/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin/`, Arm GNU 14.2.Rel1 |
| CMake | `.deps/venv/bin/cmake`, 3.31.6 |
| Ninja | `.deps/venv/bin/ninja`, Python package 1.11.1.4; executable banner `1.11.1.git.kitware.jobserver-1` |
| OpenOCD | `.deps/openocd/openocd`; configurations in `.deps/openocd/scripts/` |
| picotool | `.deps/picotool/picotool/picotool`, v2.3.1 |

`build_examples.py --pico` prepends the local CMake/Ninja and Arm compiler directories to its child process's PATH, then falls back to the existing PATH. It **always sets `PICO_SDK_PATH` to this checkout's `.deps/pico-sdk`**, overriding an external environment value. It configures `.build/pico` for `pico2`, `rp2350-arm-s`, `Debug`, Ninja and the local picotool package, then builds all twelve firmware targets with four parallel jobs. A bare `build_examples.py` invocation builds both Pico and portable host targets; explicit flags make the intended route clearer.

For interactive tool commands, set the shell separately. **Mac — project root; repeat in each new terminal:**

```sh
cd ~/hardware-from-software
export PICO_SDK_PATH="$PWD/.deps/pico-sdk"
export PATH="$PWD/.deps/venv/bin:$PWD/.deps/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin:$PWD/.deps/openocd:$PWD/.deps/picotool/picotool:$PATH"
export OPENOCD_SCRIPTS="$PWD/.deps/openocd/scripts"
rehash
command -v cmake ninja arm-none-eabi-g++ arm-none-eabi-gdb openocd picotool
arm-none-eabi-gcc -dumpmachine
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
openocd --version
picotool version
git -C "$PICO_SDK_PATH" rev-parse HEAD
```

The executable paths must resolve beneath this checkout's `.deps`; the compiler target is `arm-none-eabi`. The inspected OpenOCD reports `0.12.0+dev-gacff23f (2026-09-05-14:14)` and bundled GDB reports `15.2.90.20241130-git`. GDB's version is independent of GCC's. Keep OpenOCD's binary and scripts together.

The equivalent explicit configure/build, after those exports, is **Mac — project root**:

```sh
cmake -S examples/pico -B .build/pico -G Ninja \
  -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_BUILD_TYPE=Debug -DPICO_SDK_PATH="$PWD/.deps/pico-sdk" \
  -Dpicotool_DIR="$PWD/.deps/picotool/picotool"
cmake --build .build/pico --parallel 4
ls -l .build/pico/heartbeat.elf .build/pico/heartbeat.uf2
arm-none-eabi-readelf -h .build/pico/heartbeat.elf
```

The ELF should identify ELF32/ARM. Keep it for symbols and source debugging; copy the UF2 for BOOTSEL programming. The shared project and startup boundary are explained in [1.2](1.2-understand-the-smallest-useful-project.md).

### Recover dependencies or a stale cache

**Wrong compiler, generator, board, SDK or moved checkout:** CMake caches absolute paths and toolchain choices. Close GDB/IDE build activity first. Preserve the generated tree under a new name, then reconfigure. **Mac — project root:**

```sh
cd ~/hardware-from-software
pico_backup=$(mktemp -d "$PWD/.build/pico-before-reset.XXXXXX")
mv .build/pico "$pico_backup/pico"
python3 scripts/build_examples.py --pico
```

Use this only when `.build/pico` exists. The unique backup retains old ELF/UF2 files; rebuild success must precede any flash operation. A failed build can leave a stale UF2 at the usual path.

**SDK commit mismatch:** inspect before changing anything. **Mac — project root:**

```sh
git -C .deps/pico-sdk status --short
git -C .deps/pico-sdk rev-parse HEAD
```

Preserve SDK experiments rather than resetting them. To obtain a separate baseline, move only the SDK directory to a unique backup, then rerun setup:

```sh
sdk_backup=$(mktemp -d "$PWD/.deps/sdk-before-recovery.XXXXXX")
mv .deps/pico-sdk "$sdk_backup/pico-sdk"
python3 scripts/setup_deps.py
```

Then preserve/recreate the Pico build cache as above. Setup initialises the TinyUSB submodule needed by USB stdio; rerun it for a missing submodule.

**Checksum failure:** do not disable the check. Read the failing filename. Move that **specific cached archive** into a unique backup under `.deps`, then rerun setup to download it again. For example, only when the error names `openocd.zip`, **Mac — project root:**

```sh
archive_backup=$(mktemp -d "$PWD/.deps/download-before-retry.XXXXXX")
mv .deps/openocd.zip "$archive_backup/openocd.zip"
python3 scripts/setup_deps.py
```

A repeated mismatch needs investigation of the source/download, not an edited expected hash. An existing damaged extracted installation must also be preserved separately before setup can recreate it: the installer reuses existing directories. For a broken CMake/Ninja environment, preserve `.deps/venv` similarly and rerun setup; it installs those packages together when `venv/bin/cmake` is missing. Fix network, disk-space, clock or Python certificate errors before retrying; retain HTTPS verification.

The [official Pico VS Code extension](https://github.com/raspberrypi/pico-vscode) manages a separate tool installation. If choosing that route, use its actual reported paths and a separate build directory. Do not combine its SDK/settings with `.build/pico` or assume the script uses its SDK. Linux firmware development likewise needs host-compatible tools and explicit CMake configuration; the Mac installer is not a Linux installer. The [official SDK command-line setup](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html#quick-start-your-own-project) is the starting point for that optional environment, whose compiler combination is outside this book's pinned build evidence.

## 3. Recover Pico firmware through BOOTSEL

Use the isolated, unpowered Pico state described at the start. Leave the Pi and probe disconnected. A local GP16 LED/resistor circuit may remain; heartbeat does not drive it. Hold **Pico BOOTSEL before connecting USB**, connect the data cable to the Mac, then release BOOTSEL when the **RP2350** volume appears. This is Pico 2's volume; `RPI-RP2` is the older Pico's identity. [Official first-binary instructions](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html#your-first-binaries).

**Mac — project root; after a successful build and with the actual volume mounted:**

```sh
cd ~/hardware-from-software
ls /Volumes
ls /Volumes/RP2350/INFO_UF2.TXT
cp .build/pico/heartbeat.uf2 /Volumes/RP2350/
find /dev -maxdepth 1 -name 'cu.usbmodem*' -print
```

Do not create a missing `/Volumes/RP2350` directory. The volume should disappear when programming finishes and firmware starts. Rediscover serial devices after the restart. Replace the entire placeholder below with the Pico's actual discovered device. **Mac — project root:**

```sh
screen /dev/cu.usbmodemYOUR_SUFFIX 115200
```

Repeated `hardware-from-software heartbeat v1` lines with increasing counters are the baseline observation to make. They are expected, not measured here. USB CDC's terminal baud setting is not the GPIO UART baud. Exit screen with **Ctrl+A**, release, then **Ctrl+\**, confirming Y if asked. Ctrl+A then D merely detaches it and can leave the port owned.

After actually observing a baseline on your board, preserve both artifacts without overwriting an earlier recovery set. **Mac — project root:**

```sh
recovery_dir=$(mktemp -d "$PWD/.build/heartbeat-recovery.XXXXXX")
cp .build/pico/heartbeat.uf2 .build/pico/heartbeat.elf "$recovery_dir/"
shasum -a 256 "$recovery_dir/heartbeat.uf2" "$recovery_dir/heartbeat.elf"
printf 'Keep recovery directory: %s\n' "$recovery_dir"
```

Record the path and observed firmware identity. For later recovery, copy the UF2 from that saved directory instead of rebuilding experimental source. The ROM BOOTSEL loader remains independent of application USB; broken application USB does not require erasing a bootloader. [Pico recovery documentation](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html#reset-flash-memory).

| Symptom | Recovery check |
| --- | --- |
| Neither storage nor serial appears | Repeat hold-before-power; check a data-capable cable, hub/adapter, power and Mac accessory permission. |
| Storage disappears, no application serial | Release BOOTSEL, reconnect normally; use the saved heartbeat UF2. Firmware must initialise/service USB. |
| Serial exists but is blank/busy | Reidentify the port, close other terminals; use `screen -ls` to find your detached sessions and quit the identified session. |
| Old identity after a build | Require successful build; verify source path and copied UF2. Rebuilding does not program the board. |

Quit the monitor and unplug the Pico before reconnecting any wires.

## 4. Recover or inspect through SWD

Start with Pi–Pico wires absent and both Pico and probe unplugged. Connect probe **D**, not U, to Pico **SWCLK/GND/SWDIO**, matching the [physical connector instructions in 3.1](3.1-attach-the-debug-probe-over-swd.md). SWD supplies no target power: connect Pico USB and probe USB separately to the Mac after checking the unpowered wiring. Keep U empty. To use `blink_registers`, retain GP16 physical 21 → 1 kΩ → LED anode → cathode → GND physical 23.

**Mac — project root, terminal 1:**

```sh
cd ~/hardware-from-software
.deps/openocd/openocd \
  -s "$PWD/.deps/openocd/scripts" \
  -f interface/cmsis-dap.cfg \
  -c "set USE_CORE cm0" \
  -f target/rp2350.cfg \
  -c "adapter speed 1000"
```

`USE_CORE cm0` must precede loading the [RP2350 configuration](https://github.com/raspberrypi/openocd/blob/rpi-common/tcl/target/rp2350.cfg). Require successful Cortex-M33 examination of **`rp2350.cm0`**, not merely CMSIS-DAP identification or a listening port. The single target's GDB port is **3333**.

**Mac — project root, terminal 2; after a successful canonical build:**

```sh
cd ~/hardware-from-software
.deps/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin/arm-none-eabi-gdb \
  -q .build/pico/blink_registers.elf
```

**Pico debugger — type commands after `(gdb)`:**

```text
(gdb) set pagination off
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) monitor reset halt
(gdb) hbreak main
(gdb) continue
(gdb) delete breakpoints
(gdb) hbreak led_on
(gdb) continue
(gdb) info registers pc sp lr
(gdb) disassemble /r led_on
(gdb) x/i $pc
```

Stop on a load error. Opening the ELF supplies symbols; `load` installs its matching firmware. Letting startup and `main` run reaches `led_on` **after GPIO setup**. Do not jump the PC directly there. At that halt, [3.2](3.2-load-the-elf-and-step-through-the-led-write.md) explains instruction stepping and read-safe register checks. A halt freezes application progress and can disrupt USB servicing; it is not evidence of a firmware hang. The [OpenOCD GDB guide](https://openocd.org/doc/html/GDB-and-OpenOCD.html) documents remote programming.

To leave the installed program running, **Pico debugger, while stopped**:

```text
(gdb) delete breakpoints
(gdb) monitor reset run
(gdb) disconnect
(gdb) quit
```

If it is running under `continue`, press Ctrl+C in GDB first. Confirm breakpoint deletion when asked. Then stop OpenOCD with Ctrl+C in terminal 1. Unplug **both** USB supplies before detaching D. Stopping OpenOCD alone does not guarantee a running target.

For **probe found / target missing**, stop the server and check target power and D wiring with both devices off; try `adapter speed 100` only after those checks. For **configuration missing**, keep the explicit binary and its matching `-s` directory. For **port busy**, close your earlier server or IDE debugger. Probe firmware recovery is separate: follow [3.1's official-image procedure](3.1-attach-the-debug-probe-over-swd.md#troubleshoot-the-observed-failure), with D detached and target unpowered; `debugprobe.uf2` belongs on the accessory, not the Pico application board.

## 5. Portable host tests and native Linux applications

Host tests need a native C++20 compiler and Python 3, with no board attached. The script prefers `clang++` if found, otherwise `g++`. `--host` builds/runs the protocol tests normally and under ASan/UBSan, then compiles `hello`, `serial_client`, `button_client` and `capstone`. The PTY script tests the exact resulting serial client.

**Mac or Linux — project root:**

```sh
cd ~/hardware-from-software
python3 scripts/build_examples.py --host
python3 scripts/test_serial.py
```

Require both commands to succeed. Eleven PTY cases cover fragmented/CRLF replies, inherited software flow control, wrong IDs, device errors, oversize/NUL/bad values, LED mismatch and timeout. PTYs exercise host stream handling, not electrical UART timing. A sanitizer failure needs investigation; do not remove the sanitizer flags to obtain a passing result.

For **all seven Linux applications**, use the Pi's native compiler. The Pi may remain powered for SSH, with its header unwired or in the current lesson's unchanged state; do not run hardware clients until that lesson's ownership/wiring checks pass. **Pi — project root:**

```sh
cd ~/hardware-from-software
sudo apt update
sudo apt install build-essential git python3 file
g++ -dumpmachine
python3 scripts/build_examples.py --host --linux
python3 scripts/test_serial.py
./.build/hello
file .build/hello
```

`--linux` requires Linux and compiles every `examples/linux/*.cpp` with `g++`, C++20 and warnings as errors: `hello`, `i2c_client`, `serial_client`, `gpio_events`, `button_client`, `memory_lab`, `capstone`. It does not run peripheral clients. On the selected 64-bit Pi, the compiler target should be AArch64 Linux and `hello` an AArch64 ELF executable. Native application compilation needs distribution C/C++ development headers, **not a matching kernel module build tree**.

To rebuild one app manually, **Pi — project root**:

```sh
mkdir -p .build
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  examples/linux/serial_client.cpp -o .build/serial_client
```

Keep the relative shared headers in `examples/common/` and `examples/linux/`; copying one `.cpp` alone is insufficient. An `Exec format error` usually warrants checking `file`, compiler target and which machine built the file.

## 6. Pi modules and Device Tree build

Use this route on the **Pi**, against the common **`6.12.96-hfs1-v8`** kernel built and selected in [5.2](5.2-verify-the-linux-development-environment.md#build-the-common-kernel). Its AF_XDP support is already enabled; no kernel change is required for later networking. Modules execute in kernel context; native userspace builds above are a different operation. Keep the complete native source/output under `.build/pi-kernel/` so that the installed release's `build` link stays valid. [Linux kbuild](https://docs.kernel.org/6.12/kbuild/modules.html) requires a matching configured build, including symbol-version data when used.

**Pi — project root; inspect each result before building:**

```sh
cd ~/hardware-from-software
uname -m
uname -r
cat /proc/version
sudo apt install build-essential kmod file device-tree-compiler
readlink -f "/lib/modules/$(uname -r)/build"
running_release=$(uname -r)
header_release=$(make -s -C "/lib/modules/$running_release/build" LOCALVERSION= kernelrelease)
printf 'Running: %s\nBuild tree: %s\n' "$running_release" "$header_release"
test "$running_release" = 6.12.96-hfs1-v8
test "$running_release" = "$header_release"
test -s "/lib/modules/$running_release/build/Module.symvers"
grep '^CONFIG_XDP_SOCKETS=y$' "/lib/modules/$running_release/build/.config"
```

Continue only after every check succeeds and the native compiler is appropriate. Distribution `linux-headers-rpi-v8` packages do not target this custom release. Do not point `build` at unrelated headers or install a stock kernel/header pair as a substitute. If the retained source/output trees were moved, restore their original paths and repeat the checks. If they were deleted or the installation is partial, stop and inspect the [5.2 installation/recovery record](5.2-verify-the-linux-development-environment.md#install-the-common-kernel). The installer deliberately refuses conflicting or partial installations; running it again is not an automatic repair. Preserve the recorded boot/config artifacts and diagnose the exact refusal before changing anything. If the stock recovery kernel is currently running, restore the common kernel selection and repeat the post-boot checks before continuing modules. Never force-load a mismatch.

After a match, **Pi — project root:**

```sh
make -C "/lib/modules/$(uname -r)/build" LOCALVERSION= M="$PWD/examples/kernel" W=1 modules
mkdir -p .build
dtc -@ -I dts -O dtb -o .build/hardware-button.dtbo \
  examples/kernel/hardware-button-overlay.dts
file examples/kernel/hello.ko examples/kernel/mybutton.ko
modinfo -F vermagic ./examples/kernel/hello.ko
modinfo -F vermagic ./examples/kernel/mybutton.ko
```

The complete shared `examples/kernel/Makefile` is:

```makefile
{{#include ../examples/kernel/Makefile}}
```

It builds **both modules**. Building neither loads a module nor enables an overlay. The `.ko` is a kernel-loader input, not an application or Pico image. `dtc -@` emits overlay reference information; this DTS needs no C-preprocessor step. Module vermagic begins with the complete matching release; never force-load a mismatch. See [8.2](8.2-build-and-load-the-smallest-external-module.md) for controlled hello insertion/removal, and [8.3](8.3-describe-the-device-and-bind-the-driver.md) for overlay installation, binding verification and GPIO ownership. Do not load `mybutton` merely to test compilation.

For stale artifacts after restoring the matching build tree, optional **Pi — project root**, with the lesson's modules unloaded and generated files disposable:

```sh
make -C "/lib/modules/$(uname -r)/build" LOCALVERSION= M="$PWD/examples/kernel" clean
```

This removes generated artifacts for both modules, retaining sources. Rebuild and inspect again. Read the new kernel log for `Invalid module format`, unknown symbols or signature refusal; these have different causes. Do not force insertion/removal or disable signature checks.

## 7. Permissions, lost SSH and boot recovery

**Build permission denied:** inspect `ls -ld . .build` in the project root. Use a user-owned checkout and build without sudo. Identify the exact wrongly owned path before repairing ownership; do not recursively change system directories or use `chmod 777`.

**UART permission denied or unexpected traffic:** **Pi — working directory `~`**, inspect:

```sh
ls -l /dev/serial0
readlink -f /dev/serial0
ls -l /dev/ttyAMA0
id
cat /proc/cmdline
systemctl is-active hciuart.service
systemctl list-units --all 'serial-getty@*'
sudo fuser -v /dev/ttyAMA0
```

The selected Pi 4 route expects `serial0` → `ttyAMA0`, no console/getty/Bluetooth owner, and one application reader. Install `psmisc` if `fuser` is missing. A kernel console need not appear as a process owner. If the actual node's group is `dialout` and membership is missing, **Pi — `~`**:

```sh
sudo usermod -aG dialout "$USER"
```

Log out of all Pi SSH sessions, reconnect, and check `id` again. [7.2](7.2-configure-the-pi-uart-and-test-an-echo.md) contains the complete reversible PL011/disable-bt/console setup; changing permissions cannot fix a conflicting owner. For GPIO busy errors, use [5.3's discovery](5.3-establish-gpio-identity-and-ownership.md) and stop the identified owner normally. For `/dev/mybutton`, use [8.5's targeted udev policy](8.5-expose-dev-mybutton-to-a-c-client.md); do not make it world-writable.

**SSH name lookup fails:** use the Pi's current address from your router's client list; confirm that the Mac and Pi can communicate on the same local network. **Mac — working directory `~`**, set your actual username and discovered address (placeholders below):

```sh
PI_USER='your-pi-username'
PI_HOST='your-pi-address'
ssh "$PI_USER@$PI_HOST"
```

A timeout points first to boot, power or network reachability. Refusal at a confirmed address points to SSH not listening. With a trusted local Pi console, `sudo systemctl enable --now ssh` restores an intentionally enabled SSH service. For authentication failures, check the username and the selected key as described in [5.1](5.1-prepare-raspberry-pi-os-and-ssh.md). Do not discard host-key warnings without verifying the machine's identity. [Official remote-access guidance](https://www.raspberrypi.com/documentation/computers/remote-access.html).

**Boot fails after the button overlay:** if SSH still works, close your clients and unload only the `mybutton` module you inserted, then remove only this lesson's `dtoverlay=hardware-button` directive/comment from `/boot/firmware/config.txt` using `sudoedit`. Preserve other settings and included files. Reboot and verify `/sys/firmware/devicetree/base/hfs_button` is absent before deleting the lesson-installed `.dtbo`. Full cleanup is in [8.3](8.3-describe-the-device-and-bind-the-driver.md#troubleshooting-and-next-state). A firmware-applied overlay is part of the boot tree; `dtoverlay -r` does not undo it.

Without SSH, use a local console if available. Otherwise stop the Pi and remove its supply before moving the microSD card to the Mac. If a hung machine cannot shut down cleanly, removing power is a last resort and may lose pending writes. On the **Mac**, inspect `diskutil list` and Finder to identify the actual mounted FAT boot partition. Open **that volume's `config.txt`** in a plain-text editor and remove only the added overlay directive. The Pi path `/boot/firmware/config.txt` is not a Mac pathname. Do not format the card, alter unrelated root arguments, or assume an old full-file backup includes later changes. Eject the card through Finder, return it to the unpowered Pi, then boot and repeat the environment/ownership checks. The [official boot configuration documentation](https://www.raspberrypi.com/documentation/computers/config_txt.html) explains the files and conditional sections.

**Common-kernel boot recovery:** 5.2 retains the packaged boot files and records a pre-install configuration backup. Remove only the book-kernel selection block and restore any stock `auto_initramfs` lines that the installer commented to return temporarily to those stock files, preserving other configuration. If offline, edit the actual mounted FAT boot volume on the Mac as described above, then eject before returning it to the Pi. This is emergency recovery; restore and verify `6.12.96-hfs1-v8` before resuming the book. The common kernel's private boot artifacts, including the custom button overlay, live under `/boot/firmware/hfs-kernel/`; do not delete the stock recovery files.

For UART rollback, [7.2's recorded configuration and service backup](7.2-configure-the-pi-uart-and-test-an-echo.md#diagnose-and-leave-the-next-state) is essential: restore only changes made for that route, including the prior `hciuart`/getty states. A blanket Bluetooth restart while the PL011 is still assigned to the header would not restore a consistent setup.

## Evidence and final state

The [verification record](verification.md) distinguishes source review, compilation and execution. All twelve Pico targets compiled; portable parser normal/sanitized tests and eleven serial PTY cases passed. The common kernel is `6.12.96-hfs1-v8`; consult the current verification record for its kernel/module build and Linux application evidence. Earlier official-header compilation is historical evidence, not a substitute for rebuilding against this configuration. The modules were **not loaded into the runner kernel**. Physical flashing, SWD, GPIO/bus transfers and Pi driver runtime remain unexercised. Commands here describe procedures for the reader, not a fabricated hardware transcript.

Leave no abandoned serial reader, GDB session or OpenOCD server. Close clients before the appropriate module cleanup. For wiring changes or storage removal, shut down the Pi, wait for shutdown, and remove both boards' supplies. Keep sources, environment notes and recovery artifacts; choose the next lesson's wiring only after returning to this unpowered state.

## References

- [Raspberry Pi Pico SDK 2.3.1](https://github.com/raspberrypi/pico-sdk/releases/tag/2.3.1) and [matching tool bundles](https://github.com/raspberrypi/pico-sdk-tools/releases/tag/v2.3.1-0).
- [Raspberry Pi first binaries and BOOTSEL](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html#your-first-binaries).
- [OpenOCD GDB integration](https://openocd.org/doc/html/GDB-and-OpenOCD.html).
- [Linux external-module kbuild](https://www.kernel.org/doc/html/latest/kbuild/modules.html).
