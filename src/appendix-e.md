# Appendix E — References and glossary

Use this page to find the authority for a register, API or term, and to distinguish a checked program from a measured circuit. [Appendix A](appendix-a.md) supplies pinouts and circuit states; [Appendix B](appendix-b.md) supplies build and recovery commands. The definitions below point back to the example that gives each term a concrete meaning.

## Choose the source that answers your question

A board datasheet identifies contacts and components. A chip datasheet defines registers and electrical limits. SDK headers describe the software interface to those registers. Linux documentation defines a kernel or userspace API. None alone establishes that your actual wiring works.

The **frozen baseline** is the particular source/tool combination behind this book's compilation evidence. A **current reference** is a maintained document that may change after publication. Use current documents for corrections and errata, but compare versions before substituting their instructions into a frozen build.

### Pico hardware, firmware and debugging

| Question | Primary source and how to use it |
| --- | --- |
| Which Pico do I have, and where is a contact? | [Official Pico-series documentation](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html) distinguishes board variants. The [Pico 2 datasheet](https://datasheets.raspberrypi.com/pico/pico-2-datasheet.pdf) supplies the board pinout and schematic. These support the physical tour and Appendix A; they do not define every RP2350 register. |
| What does a register bit do? | The [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf), current release 8 when checked, defines PADS_BANK0, IO_BANK0, SIO, atomic access and silicon errata including E9. Search the block name and register name used in [2.2](2.2-read-the-rp2350-register-map.md). Recheck the document's release and your silicon revision when diagnosing inputs. |
| Which SDK implementation did these examples use? | [SDK 2.3.1 at commit `079c6f3…`](https://github.com/raspberrypi/pico-sdk/tree/079c6f39023649b154152db30f1d781e884879bc) is frozen. Its `src/rp2350/hardware_regs` and `hardware_structs` describe this chip; `src/rp2_common` supplies drivers. The [maintained SDK API reference](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html) is useful for navigation; resolve a version-sensitive question against the installed headers. |
| Why are probe discovery and target discovery different? | [Debug Probe documentation](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html) explains the accessory, D/U connectors and separate target power. [3.1](3.1-attach-the-debug-probe-over-swd.md) adds the selected OpenOCD configuration and exact connection checks. |
| What did exception entry save? | [Arm Cortex-M33 Devices Generic User Guide, issue 05, §3.3.7](https://documentation-service.arm.com/static/641b2ec1de84571cc27f3ca3) defines stacking and EXC_RETURN. The [Armv8-M Exception Model User Guide](https://documentation-service.arm.com/static/64c7832738511951cb7a246e) explains security and floating-point qualifications. Follow [4.4](4.4-inspect-exception-entry-and-return.md) with the actual LR, stack selection and disassembly; a Cortex-M4 frame recipe is insufficient. |

### Pi configuration and Linux interfaces

| Question | Primary source and how to use it |
| --- | --- |
| Which header contact or boot setting is involved? | The [Pi 4 reduced schematic](https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-reduced-schematics.pdf) identifies J8 connections. [Raspberry Pi UART configuration](https://www.raspberrypi.com/documentation/computers/configuration.html#configuring-uarts) explains routing and console ownership. For I²C3, use the [frozen overlay README](https://github.com/raspberrypi/linux/blob/ae161617d7a4552fa91d03626b8d9f3696d17481/arch/arm/boot/dts/overlays/README), together with [6.1](6.1-i-c-on-the-wires.md)'s clock-stretching rationale. |
| Which OS and headers should I compare? | [Official OS downloads](https://www.raspberrypi.com/software/operating-systems/) lists releases; [Pi kernel documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html) explains kernel builds. This book establishes its AF_XDP-enabled `6.12.96-hfs1-v8` kernel once in [5.2](5.2-verify-the-linux-development-environment.md); module builds reuse that completed build tree. [Linux 6.12 external-module instructions](https://www.kernel.org/doc/html/v6.12/kbuild/modules.html) explain kbuild. Match the reader's full `uname -r`, rather than assuming that the book's compiled module fits. |
| What identifies a userspace GPIO line? | [Linux 6.12 GPIO v2 character-device API](https://www.kernel.org/doc/html/v6.12/userspace-api/gpio/chardev.html) defines chips, chip-local offsets, line requests and edges. Use it with `gpio_events.cpp` and [5.3](5.3-establish-gpio-identity-and-ownership.md), rather than treating `/dev/gpiochip0` as permanent. |
| How does a driver acquire the named button? | The [GPIO descriptor consumer interface](https://www.kernel.org/doc/html/v6.12/driver-api/gpio/consumer.html) and [GPIO mappings](https://www.kernel.org/doc/html/v6.12/driver-api/gpio/board.html) connect `button-gpios` to `devm_gpiod_get(..., "button", ...)`. The [generic IRQ API](https://www.kernel.org/doc/html/v6.12/core-api/genericirq.html) explains threaded interrupts. |
| What controls deferred work and removal lifetime? | [Workqueue documentation](https://www.kernel.org/doc/html/v6.12/core-api/workqueue.html) explains delayed work; [kref rules](https://www.kernel.org/doc/html/v6.12/core-api/kref.html) explain retained objects; [devres](https://www.kernel.org/doc/html/v6.12/driver-api/driver-model/devres.html) explains device-managed resources. Read these together with [8.6](8.6-make-failure-and-teardown-behaviour-explicit.md): retaining an offline object does not retain its released GPIO resources. |
| What does a serial or combined I²C operation promise? | [termios(3)](https://man7.org/linux/man-pages/man3/termios.3.html), [poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html) and the [Linux 6.12 I²C device interface](https://www.kernel.org/doc/html/v6.12/i2c/dev-interface.html) explain terminal settings, readiness and `I2C_RDWR`. These are API contracts, not guarantees that the remote board replies. |
| How are virtual memory and device registers different? | [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) and [mprotect(2)](https://man7.org/linux/man-pages/man2/mprotect.2.html) explain the userspace lab. [Linux 6.12 device I/O](https://www.kernel.org/doc/html/v6.12/driver-api/device-io.html) explains mapping/accessors. The [frozen Pi pinctrl driver](https://github.com/raspberrypi/linux/blob/ae161617d7a4552fa91d03626b8d9f3696d17481/drivers/pinctrl/bcm/pinctrl-bcm2835.c) is the concrete controller implementation traced in [9.3](9.3-trace-a-real-kernel-mmio-mapping.md). |

Versioned Linux documentation describes the 6.12 API family; the frozen Raspberry Pi commit identifies the implementation built for the course kernel. A moving `latest` page or `rpi-6.12.y` branch can help discover changes, but does not identify that implementation.

### Optional sensor and measurement tools

[Bosch's BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf) answers chip-ID, register, forced-conversion, calibration and I²C/SPI questions. Its compensation appendix and [official SensorAPI](https://github.com/boschsensortec/BME280_SensorAPI) are the cross-check for [C.3](appendix-c-3.md). [Adafruit's breakout pinouts](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/pinouts) answer the separate board-level question: which contact on product 2652 reaches which signal? Do not substitute a different breakout's labels.

[Appendix D](appendix-d.md) gives the optional Saleae Logic 8/Logic 2 path and focused official setup/decoder references. A decoded transaction is evidence about sampled signals; it does not establish analogue margins or sensor accuracy. No capture in this book is presented as an author-recorded physical measurement.

## Glossary in the context of the examples

### Contacts and electrical state

| Term | Meaning here |
| --- | --- |
| Physical pin, GP, BCM, line offset | A physical pin is a connector position. Pico **GP16** is its GPIO signal at physical 21; Pi **BCM17** is a different chip's signal at physical 11. A Linux line offset identifies a line only together with its GPIO controller. See [Appendix A](appendix-a.md). |
| Controller, target, peripheral | A hardware controller performs transfers or manages signals. In the main I²C lab the Pi initiates transfers and the Pico responds as target 0x42. In Appendix C the Pico initiates transfers to the sensor. “Peripheral” can mean an on-chip block or an attached device; name which one. |
| Pad and mux | A pad is the chip's electrical interface to a signal contact. The mux selects which internal function reaches it. `blink_registers.cpp` configures both the GP16 pad and its SIO function before enabling output. |
| Ground and logic level | Ground is the shared voltage reference. A logic high/low is a voltage interpreted against electrical thresholds, not a C++ Boolean stored in RAM. Separate USB-powered boards share signal ground in the inter-board labs. |
| Pull-up, open-drain, active-low | A pull-up biases an otherwise undriven node high. Open-drain signalling drives low or releases the node, letting pull-ups raise it. Our button is physically active-low: pressing connects it to ground. GPIO descriptor polarity can translate physical low into logical asserted. |
| Debounce | Filtering contact chatter rather than treating every edge as a press. `button_irq.cpp` accepts a sampled candidate after at least 20 ms without an observed change. `mybutton.c` reschedules delayed work after edges, then takes a later single logical sample; this does not prove continuous stability throughout the delay. See [8.4](8.4-convert-gpio-events-into-driver-state.md). Neither duration is a measured universal bounce bound. |

### Registers, interrupts and execution

| Term | Meaning here |
| --- | --- |
| MMIO | Memory-mapped I/O: an address access reaches a device register. The store in `led_on()` is a device operation, not a write to ordinary application data. See [2.3](2.3-replace-gpio-calls-with-register-accesses.md). |
| `volatile` | A language qualifier used by the SDK's register types to preserve required accesses under the compiler's rules. It supplies no mutual exclusion or general inter-thread synchronization. The IRQ/main handoff still needs its critical section. |
| SET/CLR; W1C | SET/CLR registers or supported aliases change selected bits without a software read-modify-write. **Write one to clear** means a written 1 acknowledges that status bit; 0 leaves it alone. `gpio_isr()` writes the pending edge mask directly to W1C latches. These are device-specific semantics, not ordinary RAM assignment. |
| IRQ, NVIC, ISR, exception | An IRQ is an interrupt request; the Cortex-M33 NVIC manages external interrupt delivery. An ISR handles it through the processor's exception mechanism. An exception is broader than an IRQ and includes synchronous faults. Peripheral status, NVIC pending and active state are distinct stages. |
| Exception frame and EXC_RETURN | A frame is state saved during exception entry. EXC_RETURN is the handler's special return token describing return context and frame properties. Basic/extended frames, MSP/PSP and security state matter before interpreting stack words in [4.4](4.4-inspect-exception-entry-and-return.md). |
| Critical section | An interval excluding specified competing execution. The Pico handoff masks interrupts and restores their prior state on one application core. The Linux driver uses locks appropriate to its contexts. Neither technique automatically excludes every other CPU or device. |
| Virtual/physical address; I/O accessor | A process pointer is a virtual address interpreted through mappings. A physical/device address is not thereby a valid process pointer. Linux drivers map owned resources and use accessors such as `readl()`/`writel()` with documented ordering, rather than borrowing the Pico pointer technique. See [9.4](9.4-explain-ordering-access-semantics-and-ownership.md). |

### Linux lifetime and protocol state

| Term | Meaning here |
| --- | --- |
| Device Tree (DT), overlay | DT describes hardware relationships and resources; an overlay modifies that description. `hardware-button-overlay.dts` supplies the compatible string and named GPIO properties. Compiling its `.dtbo` does not apply it or bind a driver. |
| Descriptor | A kernel `struct gpio_desc *` refers to an acquired GPIO with its polarity/configuration. A userspace **file descriptor**, such as the integer from `open("/dev/mybutton", ...)`, is a different kind of handle. |
| ABI | An application binary interface defines the boundary a compiled program uses. Our device's text records, read errors and poll readiness form part of its userspace contract; kernel internal APIs have a separate version-sensitive contract. See [8.5](8.5-expose-dev-mybutton-to-a-c-client.md). |
| Per-open; latest state | Each independent open gets its own last-seen sequence; duplicated descriptors share one open file description. `mybutton` reports the newest sequence/LED state when read. Several accepted presses can coalesce into one record; this is not a queue containing every edge. A new open waits for a subsequent accepted press. |
| `poll`; deadline | `poll()` waits for readiness or error; readiness does not promise a complete application message. A deadline bounds total elapsed transaction time. `Serial::transact()` uses one monotonic two-second endpoint across partial writes and reads, rather than granting a new two seconds per byte. |
| Bus protocol; framing | I²C defines addressed clocked transfers and ACK/NACK; repeated START joins messages without an intervening STOP. SPI shifts bits with clock and selection but has no general I²C-style ACK. UART supplies asynchronous characters; our newline, request ID and length limit define the application framing above it. |
| Snapshot; calibration; compensation | A snapshot keeps related bytes from one coherent state, as in the Pico I²C register interface. Calibration coefficients describe a particular sensor; compensation converts raw readings using them. The BME280 examples implement temperature compensation only. Neither this nor the Pico's internal chip-temperature estimate establishes calibrated ambient temperature. |

The linked lessons explain the mechanisms and constraints behind these short definitions. In particular, “atomic register write,” “locked state” and “reference counted” describe particular operations and lifetimes; they are not a proof that the whole system is race-free.

## Software and hardware verification matrix

The [machine-readable verification record](verification.json) records tool pins, checks and source hashes. [Independent page reports](https://github.com/EthanHosier/hardware-from-software/tree/main/reviews) associate reviewed Markdown with its included sources and dependencies. A matching hash establishes which bytes were checked; it does not establish hardware behaviour.

The verification record retains the original 31-file example inventory and its historical checks. The [single-kernel CI run](https://github.com/EthanHosier/hardware-from-software/actions/runs/43330786612) built the full `6.12.96-hfs1-v8` Image, modules and Device Trees, staged module installation, and compiled both external modules against that completed output. Its configuration delta enables AF_XDP and gives the kernel its own release name; compiler metadata and a newly visible disabled diagnostic option are recorded separately. All seven Linux applications compiled natively on hosted Arm64 Bookworm. Fifteen installer fixture tests exercise file/configuration handling without installing or booting a Pi. Consult the downloadable record for exact source and artifact hashes.

| Artifact or activity | Evidence actually obtained | What remains unexercised |
| --- | --- | --- |
| Lesson sources and examples | Source/API review; page reports record exact reviewed hashes and limits. | Source review does not execute the programs or establish race freedom. Publication status is tracked separately on the [roadmap](roadmap.md). |
| Twelve Pico firmware targets | Cross-compiled and linked on Apple Silicon macOS for `pico2`, `rp2350-arm-s`, SDK 2.3.1 with Arm GNU 14.2.Rel1. | Flash/boot, USB output, register-to-pin behaviour, LED/button operation, interrupts and SWD attachment/stepping. |
| Shared protocol parser | Host compiled and executed normally and with AddressSanitizer/UndefinedBehaviorSanitizer. | Actual UART reception and every possible input/interleaving; sanitizers are not a correctness proof. |
| C++ serial client | Eleven PTY integration cases passed on macOS and hosted Linux, including inherited IXON/IXOFF/IXANY clearing, fragmented replies, validation failures and deadline. | Pi UART routing, electrical signalling and Pico firmware responses. |
| Inline diagnostics in 7.2 and 10.2 | Separate macOS PTY suites: four echo-helper cases in 7.2; four raw-helper cases in 10.2 (fragmented success, wrong reply, oversize, silent two-second deadline). Flag/restoration checks passed. | These helpers' physical UART procedures, reset/reconnection and end-to-end recovery. See the respective independent review reports for exact scope. |
| Seven Linux C++ applications | Native Arm64 userspace compilation on hosted Linux CI. Earlier Arm64 cross-compilation is recorded separately. Userspace builds use distribution development/UAPI headers, not the Pi kernel-module build tree. | Hardware-facing application execution on a Pi, including the capstone. |
| `memory_lab` | Executed on hosted Ubuntu Linux: mappings/page-touch loop and child-only protection fault; parent reported expected SIGSEGV and exited successfully. | A physical Pi run; no fixed page-fault count or physical-address claim follows. |
| `hello` and `mybutton` modules | Module compilation against the completed **6.12.96-hfs1-v8** course-kernel build, with its exact generated configuration and symbol versions. | Neither module was loaded into the hosted runner kernel or a Pi kernel. Probe, IRQ, blocked reads, removal/unbind and failure-unwind runtime remain unexercised. |
| Button Device Tree overlay | Compiled with `dtc -@` in hosted Linux CI. | Applying it at boot, binding the actual device and verifying GPIO ownership. |
| `mmio_fragment.c` | Source-reviewed contextual probe fragment only; **no module target and no compilation claim**. | Mapping/access on any device. Compiled `hello`/`mybutton` modules do not validate this fragment. |
| Optional BME280 examples | Both firmware targets compiled, shared compensation source reviewed, C.3's explicitly synthetic arithmetic independently reproduced on a host. | Physical I²C/SPI, real calibration bytes, sensor temperature, timing and ambient accuracy. Pressure/humidity compensation is not implemented. |
| All physical bench work | **Physical runtime not exercised:** no verified board connection in the authoring environment. | Board boot, GPIO/electrical levels, SWD, UART/I²C/SPI, analyser capture, kernel runtime/unbind, LED output and physical timing/accuracy measurements. |

The frozen firmware tool record is SDK **2.3.1**, commit `079c6f39023649b154152db30f1d781e884879bc`; Arm GNU **14.2.Rel1 (14.2.1)**, **darwin-arm64** host bundle; CMake **3.31.6**; Ninja Python package **1.11.1.4** (the executable banner is `1.11.1.git.kitware.jobserver-1`). Appendix B records paths and recovery.

For a new Pi image, the selected baseline is intentionally **Raspberry Pi OS (Legacy) Lite, 64-bit, Debian 12 Bookworm**, release **6 October 2026**, kernel 6.12 family. The official image description/hash was checked; the image was **not installed or booted** during authoring. It is separate from the current Trixie release and from the reader's running kernel. The compiled-header package's changelog records source commit `ae161617d7a4552fa91d03626b8d9f3696d17481`, used for the frozen controller links above; it does not identify an uninspected Pi's kernel.

Outputs described as expected in the lessons are predictions for reader checks, not captured board measurements. To add physical evidence, record the actual board/revision, OS/kernel/tool versions, source identity, wiring state, procedure and observation. Keep a failed or unperformed check visible instead of promoting compilation to runtime success.

## Attribution and reuse

The prose and labelled teaching diagrams belong to this course's source tree; external references remain their authors' work. A link is not permission to redistribute a document or image. Preserve the notices and licences supplied with dependencies and copied code: consult the [pinned SDK licence](https://github.com/raspberrypi/pico-sdk/blob/079c6f39023649b154152db30f1d781e884879bc/LICENSE.TXT), the [matching Linux copying notice](https://github.com/raspberrypi/linux/blob/ae161617d7a4552fa91d03626b8d9f3696d17481/COPYING), and the [Bosch SensorAPI licence](https://github.com/boschsensortec/BME280_SensorAPI/blob/master/LICENSE). Do not infer a blanket redistribution licence for the book or every dependency from a module's `MODULE_LICENSE("GPL")` declaration.
