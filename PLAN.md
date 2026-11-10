# Hardware from Software — research and chapter plan

Planning date: 10 November 2026. Updated to include the requested detailed visual hardware tours. Status: proposed curriculum, ready for authoring when requested. This file is the plan only; no mdBook, example programs, repository, hosting project, or deployment has been created.

## Intended reader and result

A personal laboratory guide for an experienced C++/Rust programmer who understands operating-system fundamentals but wants direct experience of instructions, registers, wires, interrupts, buses, and Linux device drivers. Use the concise explanatory style and ordinary mdBook reading interface of `/Users/hermes/Desktop/learncpp-mirror`. Explain unfamiliar electronics and embedded details completely; do not reteach introductory C++.

The book follows the shared conversation's exercises: setup → register-controlled LED → button interrupts → I²C/SPI → Pi–Pico UART → Linux GPIO driver → virtual memory/MMIO. Debugging is introduced before interrupts, and Pi setup is delayed until it is needed. A short final integration exercise joins the pieces.

Use C++ for Pico firmware and Linux applications. Use C for the Linux kernel module, in line with the kernel's supported toolchain and APIs. Arm Cortex-M33, one application core, and no RTOS form the main Pico route. RISC-V, multicore, DMA, PIO, PCIe, and Rust ports are possible later extensions, not prerequisites.

Working title: **Hardware from Software — Pico 2 and Raspberry Pi 4**.

## Hardware evidence and assumptions

The shared conversation identifies:

- MacBook Pro with M4 Pro, running macOS.
- Raspberry Pi Pico 2 with headers; use the non-wireless board as the baseline, with an explicit identification check.
- Raspberry Pi 4 Model B; the prior assistant's unusual “3GB” description is unverified and will not be repeated as a fact. Read the actual model and memory on the machine.
- Raspberry Pi Debug Probe.
- Official Pi 4 USB-C supply, 64GB Raspberry Pi OS microSD, microSD-to-SD adapter, and a micro-USB data cable.
- ELEGOO Electronics Fun Kit: the conversation identifies a breadboard, resistors, LEDs, buttons, and jumper wires, but does not establish the exact kit revision or every component.

Do not assume an I²C sensor, SPI peripheral, logic analyser, multimeter, USB-A adapter/hub, or second micro-USB data cable is present. The Mac, Pico, and probe connection layout must account for two USB devices and the Mac's USB-C ports. Confirm the supplied probe cable and SWD connector fit; do not silently require soldering.

The main route uses the two existing boards for I²C and the Pico's internal temperature sensor, so an external sensor purchase does not block progress. This is an explicit adaptation of the original “Pico reads a sensor” suggestion: here the Pi reads a sensor-like register interface implemented by the Pico. An optional external-sensor route preserves the original experiment directly.

## Teaching contract for every practical lesson

Each lesson will contain, where applicable, in this order:

1. The physical result and the relevant prerequisite lesson.
2. Exact parts, board state, and files used; state what is being reused or replaced.
3. The necessary concept, explained before the unfamiliar operation.
4. A schematic and wiring table using both GPIO names and physical pin numbers. Breadboard orientation and four-leg button connectivity must be clear.
5. Numbered actions and complete compilable files, with the working directory and execution machine labelled **Mac**, **Pi**, or **Pico debugger**.
6. Build, flash/load, and run commands; show the role of UF2 versus ELF.
7. Expected output and a concrete physical or debugger observation. Label sample output as illustrative unless actually recorded.
8. A short explanation tracing software to the hardware event.
9. Troubleshooting organised by observed symptoms, followed by cleanup and the exact state needed for the next lesson.

No quizzes. Small “change this and observe” experiments should include their expected interpretation, rather than hide essential instructions in a question. Complete examples live alongside the prose, with source includes or an equivalent consistency check to keep displayed code aligned with downloadable code.

Use SDK startup, linker support, and clock initialisation initially. Be explicit about that boundary: register-level peripheral programming does not mean writing the entire boot process from scratch. Start peripheral examples with a brief SDK baseline, then expose registers and instructions at the relevant point. No placeholder register addresses in runnable examples.

## Exact chapter and lesson outline

There are **44 main lessons across 11 chapter groups (0–10)**, plus appendices and optional extensions. These are focused reading pages, not 44 separate hardware projects.

### 0. Know your hardware and your bench — 6 lessons

This chapter is a visual introduction to the physical hardware, before installation or programming. Its purpose is to let the reader look at each board or component, recognise its parts, and understand what each part does and when the book will use it. Do not assume electronics familiarity just because the reader knows C++ and operating systems.

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 0.1 What you are going to build | Map the seven original exercises onto this book. Contrast a Linux process, kernel driver, bare-metal firmware, processor, peripheral controller, and physical component. Show the eventual Mac → SSH → Pi → UART → Pico → LED chain, with photographs or accurate board silhouettes identifying the actual devices. | Reader can identify which processor runs each program and where Linux is involved. |
| 0.2 A labelled tour of your Pico 2 | Provide an annotated top view of the actual baseline board and a separate internal block diagram. Label the RP2350 microcontroller package, external flash chip, USB connector, BOOTSEL button, onboard LED, GPIO headers, power/ground pins, SWD connector, and the relevant clock/power circuitry. Explain processor cores, SRAM, and peripheral controllers as parts inside the RP2350 rather than separate visible chips. Distinguish flash from SRAM, the chip from the board, and a physical pin from its selectable peripheral functions. Explain what each labelled item does and when we use it. Identify board orientation and Pico 2 versus Pico 2 W differences explicitly. | Reader can point to the main physical parts on their Pico, explain where firmware and working data live, and identify power, GPIO, and debug connections. |
| 0.3 A labelled tour of your Raspberry Pi 4 | Provide annotated top and underside views. Label the BCM2711 system-on-chip package, RAM package, USB controller, Ethernet controller/connector, USB ports, USB-C power input, micro-HDMI ports, microSD slot, 40-pin GPIO header and pin-1 marker, wireless section, and visible display/camera connectors. Explain SoC versus CPU versus board, RAM versus microSD storage, and controllers versus physical connectors. Cover supporting power/clock circuitry at a useful overview level, without trying to name every passive component. State which parts are used in our labs and which can be left alone. Account for a heatsink or case obscuring parts without requiring removal. | Reader recognises their board's main components and ports and can distinguish power, storage, networking, and GPIO connections. |
| 0.4 A labelled tour of the Debug Probe and cables | Annotate the probe's USB, SWD, and UART connections and their signal names; explain the probe's own controller separately from the target Pico. Show the actual connector orientation and the path each cable serves. Inventory data-capable micro-USB cables, USB-C adapters/hub, supplied SWD cable, jumper wire ends, power supply, and SD adapter. Distinguish power, data, debug, and serial connections, including the fact that the SWD connector has no target-power pin. | Reader can identify every cable endpoint and explain the separate power, programming/debug, and serial-data paths. |
| 0.5 Recognise the breadboard and kit components | Use labelled diagrams of the breadboard's hidden contact strips, centre gap, and possibly split power rails; show how the Pico straddles the gap. Identify resistors and their values, LED anode/cathode and polarity markings, four-leg button internal contact pairs, and jumper wire types. Explain what each component contributes to our circuits. Identify these against the actual kit rather than inventing included parts. | Reader can select the required components and understand which breadboard holes and button legs are electrically connected. |
| 0.6 Assemble the bench and understand the electrical rules | Consolidate the physical inventory and power/data/debug connection diagram. Explain ground/reference voltage, 3.3V logic, LED current limiting and resistor selection, pull resistors, floating inputs, and separate USB supplies. Explain why signal-ground wires are shared but power outputs are not joined. Establish consistent top-view orientation, GPIO names versus physical pin numbering, and a habit of powering down before changing wiring. | Reader can identify all equipment, interpret later wiring tables, and assemble the LED circuit correctly before firmware-controlled output is enabled. |

#### Required diagrams and visual accuracy

- Each device tour includes an annotated physical view plus a short table: **label → what it is → what it does → when we use it**. Keep labels tied to recognisable physical landmarks.
- Use accurate, source-checked photographs or vector drawings of the correct board revision. Label underside views as underside views; never silently mirror a pinout. Prefer original SVG diagrams and appropriately licensed official imagery. Do not use generated board imagery as the source of truth for component placement, connector orientation, or pin numbering.
- Include separate conceptual block diagrams for what is inside a chip. Explicitly distinguish these from diagrams of visible board components; SRAM and processor cores must not appear to be separately identifiable chips on the Pico.
- Give connectors and pin groups readable close-ups, include pin-1/orientation markers, and use numbered callouts where direct labels would become crowded. Diagrams must remain legible on a narrow display, with a larger view available when useful.
- Include text descriptions/alt text and corresponding label tables so the explanation does not depend on colour or the image alone.
- Explain unfamiliar terms at first use. Keep component identification approachable while connecting it to the later register, interrupt, bus, and driver exercises.
- Check physical placement and labels against official schematics, board drawings, and documentation for the selected board before authoring. Where the user's revision is unconfirmed, state that and avoid pretending a reference photograph is a photograph of their exact device.

### 1. Build and flash your first Pico program — 4 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 1.1 Install a reproducible macOS toolchain | Use the checkout-local pinned setup script on Apple Silicon as the reproducible primary route; record exact versions and paths. Explain the official Pico VS Code extension as an optional alternative with separate managed tool paths. Check Arm GCC, CMake, build tool, GDB, OpenOCD, and SDK paths. Explain host tools versus target binaries. Keep manual setup and editor integration details in the recovery appendix. | Version/path checks succeed and the compiler targets Arm embedded firmware. |
| 1.2 Understand the smallest useful project | Complete CMake and C++ files; explicit Pico 2/Arm target, debug build, SDK import, SDK initialisation, and USB serial output. Explain compilation, linking, ELF, UF2, flash, SRAM, and the SDK's startup contribution. | A clean build produces a Pico 2 ELF and UF2. |
| 1.3 Flash with BOOTSEL and read output | Enter BOOTSEL, copy the UF2, observe the RP2350 mass-storage device disappear, locate the serial port, and read a versioned heartbeat. Account for macOS serial device naming and charge-only cables. | Rebooted firmware repeatedly emits its identity and heartbeat. |
| 1.4 Make the build–flash–observe loop repeatable | Change a heartbeat value, rebuild, flash, verify the new firmware, and recover from a program that no longer starts its USB interface. Establish stable example names and a version report. | A deliberate source change is visible on the actual board, and BOOTSEL recovery is understood. |

### 2. Make a CPU store change a pin — 4 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 2.1 Wire and verify an external LED | Use one conservative resistor value from the kit and a specified external GPIO. Provide the complete pin-to-resistor-to-LED-to-ground circuit. Briefly use the SDK GPIO API to establish a known-good circuit. | The external LED blinks reliably; common polarity and rail mistakes are resolved. |
| 2.2 Read the RP2350 register map | Derive the chosen pin's pad control, function selection, SIO output enable, and output registers from the RP2350 datasheet. Explain address, register width, access type, masks, reset state, pad isolation, and read-modify-write hazards. | Reader can locate each register and explain the required configuration sequence. |
| 2.3 Replace GPIO calls with register accesses | Keep SDK startup, replace GPIO setup/output with documented register operations, and use SET/CLR aliases where appropriate. Introduce volatile access and its limits; distinguish peripheral atomic operations from C++ inter-thread atomics. | The same LED blinks using direct peripheral-register accesses. |
| 2.4 Follow the generated machine instructions | Compare source, disassembly, address calculation, and the store to a peripheral register. Explain that optimisation, bus transactions, pad configuration, and physical timing are separate layers. | Reader identifies the instruction responsible for changing the LED output; live stepping follows in Chapter 3. |

### 3. Debug the running hardware — 3 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 3.1 Attach the Debug Probe over SWD | Separate SWD and probe UART connectors, verify SWDIO/SWCLK/GND, power the target separately, and run OpenOCD with RP2350 support and the correct target configuration. Account for probe firmware compatibility. | OpenOCD recognises the RP2350 target rather than merely finding the USB probe. |
| 3.2 Load the ELF and step through the LED write | Connect Arm GDB, reset/halt, load the debug ELF, break at a known point, inspect variables/registers, and step across the output store. Explain stopped-CPU effects and stale source/binary mistakes. | A debugger step produces the predicted LED state, and a register read confirms it. |
| 3.3 Diagnose a broken program systematically | Use supplied reversible faults: wrong mask, missing output enable, wrong mux, and a bounded loop. Inspect stack, PC, peripheral state, and disassembly. Explain when reading a register can have side effects. | Reader repairs each fault using evidence and restores the known-good baseline. |

### 4. From a button to an interrupt handler — 4 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 4.1 Read a button by polling | Use an active-low button and an explicitly specified external pull-up; explain contact pairs, input enable, electrical versus logical state, and the relevant silicon erratum/revision qualification. Poll an input register. | Press/release yields stable raw states, with bounce visible in transition counts where present. |
| 4.2 Configure and acknowledge a GPIO interrupt | Move from the SDK callback baseline to the relevant GPIO interrupt enable/status/acknowledge path and CPU IRQ enable. Explain edge versus level, NVIC, vector entry, and write-one-to-clear semantics. | A button edge reaches an ISR and main-loop reporting confirms receipt. |
| 4.3 Share state and debounce correctly | Keep the ISR short; no sleeping or USB logging there. Use a documented single-core handoff with appropriate critical sections or proven lock-free operations, and a timed debounce state machine. Explain why volatile alone is not synchronisation. | One accepted press toggles once; delayed main-loop work does not corrupt shared state. |
| 4.4 Inspect exception entry and return | Break in the handler, inspect the Cortex-M33 exception frame and return path, observe pending interrupts, and explain how halted execution distorts timing. Contrast polling, interrupt notification, and optional wait-for-interrupt behaviour. | Reader traces pin transition → interrupt controller → handler → resumed application. |

### 5. Bring up the Pi 4 headlessly — 3 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 5.1 Prepare Raspberry Pi OS and SSH | Support checking a preinstalled card and, when necessary, imaging a selected 64-bit Raspberry Pi OS Lite release. Configure user, hostname, network, SSH, and key authentication; use the actual Mac network route. Explain local-network discovery and the fallback to an IP address. | Mac reaches a Pi shell through SSH without a monitor. |
| 5.2 Verify the Linux development environment | Record board model, OS release, architecture, running kernel, compiler, and boot configuration layout. Install only required development packages and compile/run a tiny C++ program natively. | A machine/version record and a working native C++ build. |
| 5.3 Establish GPIO identity and ownership | Distinguish physical header pins, BCM names, gpiochip line offsets, and requested lines. Use current GPIO character-device tooling to inspect the actual controller instead of assuming a gpiochip number. Introduce line ownership and pin multiplexing. | Correct header lines are identified and confirmed available for later labs. |

### 6. Understand clocked buses with your existing boards — 4 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 6.1 I²C on the wires | Explain open-drain signalling, pull-ups, START/STOP, 7-bit addresses, R/W bit, ACK/NACK, register pointers, and repeated START. Wire Pi controller to Pico target using the documented Pi pull-ups and 3.3V signalling; separate powering and common ground. | A verified wiring table and an idle bus; with an analyser, the idle voltage and first transfer can be observed. |
| 6.2 Turn the Pico into a register-based peripheral | Implement a small versioned I²C register interface inspired by the official memory-target example. Expose identity, LED control, button count, and a coherent snapshot of the internal temperature ADC reading. Keep bus callbacks bounded and handle repeated START and multi-byte state correctly. | Known register transactions produce predictable values and LED changes. |
| 6.3 Write a Linux I²C client | Enable the intended Pi I²C bus, identify `/dev/i2c-*`, and use a complete C++ client with `I2C_RDWR` for combined transactions. Read identity/data, control the LED, and diagnose wrong addresses, NACKs, and partial/failed transfers. Distinguish an approximate chip temperature from ambient measurement. | Pi reads the Pico's sensor data and changes its LED over real I²C wires. |
| 6.4 Compare SPI with a loopback experiment | Use the Pico SPI controller in a simple loopback exercise, initially with documented loopback support or an explicit MOSI-to-MISO wire. Explain SCK, MOSI/MISO, chip select, CPOL/CPHA, full duplex, and why echo success does not validate a real peripheral's timing. Detach any earlier wiring that conflicts. | Transmitted patterns are recovered, and the reader can explain how SPI differs from I²C. |

The main I²C lab deliberately uses the Pi as controller and Pico as peripheral. The optional sensor appendix below puts the Pico in the controller role against a real external chip, matching the original suggestion exactly.

### 7. Connect Linux to bare-metal firmware over UART — 4 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 7.1 Make a UART link electrically and logically correct | Cross Pi TX to Pico RX and vice versa, share ground, and use 3.3V logic. Explain baud rate, start/stop bits, idle state, and the distinction between UART, USB serial, and RS-232. Establish a fixed 115200/8-N-1 baseline. | A verified wiring diagram and matching settings on both ends. |
| 7.2 Configure the Pi UART and test an echo | Disable the serial login console while enabling UART hardware. Resolve Pi 4 Bluetooth/PL011 versus mini-UART routing explicitly, inspect `/dev/serial0`, and verify the actual port. Keep Pico diagnostic output separate from protocol traffic. | Pi and Pico exchange an exact test pattern without console noise. |
| 7.3 Build a robust C++ serial client | Use termios, bounded buffers, timeouts/poll, partial reads/writes, interruption handling, and clean close. Implement a small newline-framed protocol with request identifiers, bounded lengths, explicit errors, and commands for LED, button count, and temperature. | Split, delayed, malformed, and multiple messages are handled predictably. |
| 7.4 Trace a command end to end | Follow an SSH-launched command through Linux write/read, the UART controller, Pico receive/parsing, the register store, and acknowledgement. Compare USB logs, optional analyser capture, and debugger evidence; distinguish throughput from latency. | One command can be accounted for at every stage from Linux process to LED. |

### 8. Write a Linux driver for a physical button — 6 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 8.1 Establish a userspace GPIO baseline | Rewire a button and LED to dedicated Pi header pins, confirm availability, and use a C++ client against the GPIO character-device v2 API to request lines and receive edges. Explain why an educational custom driver is not required for every GPIO application. | The physical circuit works through the existing Linux interface before new kernel code is introduced. |
| 8.2 Build and load the smallest external module | Install headers matching the running kernel, use kbuild, and build a minimal C module with load/unload logging. Explain module metadata, kernel/user address spaces, and recovery if the module or headers mismatch. | Module loads, logs, and unloads cleanly on the recorded kernel. |
| 8.3 Describe the device and bind the driver | Provide a complete Device Tree overlay and matching platform driver. Use a custom compatible string, named `*-gpios` properties, descriptor acquisition, explicit pin configuration, and error unwinding. Explain probe/binding and how to disable the overlay. | Driver binds to the intended device and requests the intended GPIO lines exclusively. |
| 8.4 Convert GPIO events into driver state | Map the button descriptor to an IRQ, use an appropriate threaded handler/deferred debounce design, and update protected event state. Explain hard IRQ versus process/thread context and which operations may sleep. | Accepted physical button events produce a consistent kernel counter without ISR logging floods. |
| 8.5 Expose `/dev/mybutton` to a C++ client | Use a misc device and deliberately small text-record ABI. Define blocking/nonblocking reads, poll readiness, short-buffer behaviour, errors, and permissions. Use a monotonic event counter with per-open last-seen state; explicitly document that this is a latest-state/counter interface, not an unlimited event queue. | A C++ client sleeps for an event, wakes on a press, reads a valid record, and handles timeout/nonblocking cases. |
| 8.6 Make failure and teardown behaviour explicit | Test concurrent readers under the defined counter semantics, repeated load/unload, signal interruption, a held button, bounce, and open descriptors. Define an explicit offline state and a reference-counted userspace-facing object so unbind wakes waiters and returns a documented device-gone error without freed-memory access. Compare with standard gpio-keys/input drivers. | No blocked reader remains stranded during documented teardown, and resources/pins can be reclaimed. |

### 9. Virtual memory and the registers beneath the driver — 4 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 9.1 Observe your Linux process's mappings | Inspect `/proc/self/maps`, allocate/map ordinary memory, query page size, and compare virtual addresses in separate runs. Explain why pointer values do not reveal physical addresses and why an MCU MMIO address is not a usable Linux userspace pointer. | Reader can classify mappings and explain virtual versus physical addressing. |
| 9.2 Observe demand paging and protection faults | Use a bounded userspace program to touch mapped anonymous pages and compare minor-fault counts; use a child process and `mprotect` for a controlled protection fault. Explain why observed counts vary and why normal demand faults differ from invalid accesses. | Measured paging behaviour and a predictable child-process protection fault, without crashing the Pi kernel. |
| 9.3 Trace a real kernel MMIO mapping | Inspect the live Device Tree and resource descriptions, then follow the matching Pi kernel GPIO/pinctrl source from resource lookup/mapping to register access. Explain address translation (`reg`/`ranges`), `ioremap` and managed mapping helpers, `__iomem`, device attributes, and `readl`/`writel`. Use source matching the selected kernel, not a random tutorial's physical addresses. | A concrete map from the driver's GPIO operation to the owning controller's mapped register access. |
| 9.4 Explain ordering, access semantics, and ownership | Compare Pico volatile pointers with Linux MMIO accessors, memory barriers, write-one-to-clear registers, SET/CLR aliases, resource ownership, and ordinary RAM. Walk through a minimal resource-owning mapping example as annotated source, not a module that maps over the active GPIO controller. | Reader explains why copying the Pico pointer technique into a Linux process or second GPIO driver is incorrect. |

Scope qualification: the original chat loosely suggested experimenting with `ioremap()` and page faults. The main book makes page faults a live userspace experiment and MMIO mapping a trace through a real controller driver. It will not promise a safe arbitrary-register-writing lab on a controller already owned by Linux. A separate live mapping exercise would require a specifically reserved peripheral/resource and additional validation; it is not silently assumed here.

### 10. Put the whole chain together — 2 lessons

| Page | Content and practical work | Completion evidence |
| --- | --- | --- |
| 10.1 Use a Linux device event to control Pico hardware | Reuse `/dev/mybutton` and the robust UART client. A physical Pi button event wakes the C++ process, which sends an identified LED command to the Pico and verifies its reply. Show separate Pi/Pico LEDs and all wiring on one consolidated diagram. | Physical button → Linux IRQ → driver → blocked read/poll → C++ → UART → Pico → LED. |
| 10.2 Verify, troubleshoot, and explain the system | Use a repeatable end-to-end checklist: restart the client, reset the Pico, remove/reconnect the data link with boards powered down for wiring changes, reject malformed requests, and handle the driver going offline. Revisit what each layer provides and identify the next experiment by interest. | A reproducible demonstration and a clear explanation of the complete hardware/software path. |

## Appendices and optional extensions

- **A. Pinouts and circuit reference:** larger versions of Chapter 0’s labelled board/component diagrams, complete Pico 2 and Pi 4 pin maps used by the book, resistor identification, LED polarity, button contacts, SWD/UART connectors, and one wiring-state table per project. Every GPIO reference has a board name.
- **B. Build and recovery reference:** terminal-only macOS setup, version/path report, clean rebuild, ELF/UF2, BOOTSEL, OpenOCD/GDB transcript, USB serial discovery, SSH/network recovery, kernel/header mismatch, overlay removal, and SD-card recovery.
- **C. Optional real I²C/SPI sensor:** one explicitly identified BME280 breakout supporting 3.3V logic and exposed SPI pins. Identify the breakout and headers before finalising wiring. Pages: C.1 device identity/register map; C.2 Pico I²C chip-ID and raw reads; C.3 calibration/compensation and measured data; C.4 the same register reads over SPI and comparison. Use Bosch's datasheet and reference driver as an accuracy cross-check. Do not confuse BME280 with BMP280 or assume all breakout boards expose SPI.
- **D. Optional logic analyser:** only after identifying an actual device and macOS-compatible software. Ground/threshold/sample-rate setup, digital decoding limits, and captures for GPIO, UART, I²C repeated START, and SPI. The Debug Probe is not presented as a logic analyser. No core lab depends on this accessory.
- **E. Reference and glossary:** Arm exception terms, electrical terms, register access conventions, protocol glossary, cited datasheets, kernel APIs, licences and attribution, and the validated software/hardware matrix.
- **F. Later directions:** short pointers to PIO, DMA, second-core communication, a Rust firmware port, and writing startup/linker code. These are next steps, not unfinished promises in the main curriculum.

## Implementation and publication plan, for the later build

- Create a separate project with ordinary mdBook, its sidebar/search/themes, and the same light/navy defaults as the existing book. Use numbered lessons and code highlighting; embedded C/C++ blocks are not browser-runnable.
- Use the original project's build arrangement as a reference, without copying credentials, `.env` files, private source archives, Git identity, Vercel project linkage, or its private-publication requirements.
- New original prose, local schematic assets, and complete firmware/application/module examples. Avoid a custom web framework or unnecessary backend.
- Keep firmware projects, Linux applications, and kernel examples in distinct directories. Make lesson code correspondence mechanically checkable.
- Pin tool/SDK versions after verifying the actual installation route. Record exact Raspberry Pi OS image and running-kernel/header versions for kernel examples. Do not present an untested combination as a validated baseline.
- Deploy only after authoring is authorised, to a **new Vercel project with a publicly readable production domain**. Verify the production book without an authenticated cookie or bypass token, including a nested chapter, stylesheet, search assets, and downloads. The current private LearnCpp book keeps its existing access settings.
- The website is a static guide. It does not connect to or remotely operate the user's Pi and has no reason to contain real network credentials, SSH keys, or personal host addresses.

## Verification plan and honest limits

Before publication, compile all Pico examples for the stated Pico 2/Arm board, compile Linux userspace programs for the selected Pi environment, and build the module against the selected kernel headers. Run host-side protocol/parser tests for malformed/fragmented/bounded input, and check that every referenced file and internal link exists. Check mdBook rendering on desktop and narrow screens.

Hardware-dependent results need physical evidence. LED operation, exact wiring, electrical levels, SWD attachment, interrupts, actual I²C/UART transfers, module runtime behaviour, and the capstone cannot be established merely by compiling. Mark these checks separately as hardware-confirmed, awaiting reader confirmation, or not exercised. Do not fabricate serial traces, analyser captures, physical timings, or claims of successful physical tests.

The only user-specific facts still needing inspection before exact commands/wiring are finalised are board variant/revision, actual kit components/cable availability, macOS/toolchain versions, Pi OS/kernel version, and any optional sensor/analyser. These do not block the chapter plan. No purchase or extra hardware is assumed for the main route.

## Research findings that affect the plan

1. Pico 2 is RP2350, and selecting `pico2` is material; default RP2040 examples/configurations cannot simply be copied. Use RP2350-capable OpenOCD with the corresponding target configuration. Sources: [Pico examples](https://github.com/raspberrypi/pico-examples), [getting-started guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf), [Debug Probe](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html).
2. The Pico board variant affects its onboard LED and SWD connector. An external LED makes the register exercise consistent. SWD does not supply target power. Source: [Pico-series hardware documentation](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html).
3. RP2350 register access includes pad configuration and atomic aliases, beyond the shared chat's illustrative pointer write. Source: [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).
4. Early RP2350 silicon has GPIO leakage errata; revision-aware input circuitry matters. Source: [Raspberry Pi's A4 announcement](https://www.raspberrypi.com/news/rp2350-a4-rp2354-and-a-new-hacking-challenge/) and the datasheet's errata. Check the actual stepping rather than assuming an old or corrected board.
5. The official SDK documents the internal temperature sensor and I²C peripheral facilities; official examples include memory-target, onboard-temperature, GPIO IRQ, and SPI experiments. This supports the proposed no-extra-sensor route. Sources: [hardware SDK API](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html), [official examples](https://github.com/raspberrypi/pico-examples).
6. Pi headless access needs explicit SSH configuration, and UART use needs console/routing checks, including Pi 4 Bluetooth and the mini-UART. Sources: [remote access](https://www.raspberrypi.com/documentation/computers/remote-access.html), [configuration/UART documentation](https://www.raspberrypi.com/documentation/computers/configuration.html).
7. Linux exposes I²C combined transfers through its device interface and GPIO through the character-device API. Sources: [I²C userspace interface](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html), [GPIO v2 character-device API](https://www.kernel.org/doc/html/latest/userspace-api/gpio/chardev.html).
8. Kernel module builds depend on matching kernel headers, not merely installing a compiler. Sources: [Pi kernel documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html), [external module build documentation](https://www.kernel.org/doc/html/latest/kbuild/modules.html).
9. Use GPIO descriptors, named Device Tree mappings, and the platform-driver model for the educational driver. Sources: [descriptor API](https://www.kernel.org/doc/html/latest/driver-api/gpio/consumer.html), [GPIO mappings](https://www.kernel.org/doc/html/latest/driver-api/gpio/board.html), [platform drivers](https://www.kernel.org/doc/html/latest/driver-api/driver-model/platform.html), [misc devices](https://www.kernel.org/doc/html/latest/driver-api/misc_devices.html).
10. Kernel C and userspace C++ are separate build contexts; Linux MMIO uses mapped resources and accessors. Sources: [kernel languages](https://www.kernel.org/doc/html/latest/process/programming-language.html), [device I/O documentation](https://www.kernel.org/doc/html/latest/driver-api/device-io.html). The [Pi GPIO/pinctrl source](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/pinctrl/bcm/pinctrl-bcm2835.c) was identified as a source-reading reference; the book must select the branch/commit matching its validated kernel.
11. BME280 supports both I²C and SPI, with specific interface selection and compensation rules; the breakout remains an explicit extra. Source: [Bosch BME280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf).
12. Vercel protection scope distinguishes production-domain access from other deployment URLs. For this project the acceptance criterion is a stable production URL that works without login. Source: [Vercel deployment protection](https://vercel.com/docs/deployment-protection). Recheck the actual dashboard/API behaviour during deployment rather than inheriting the old project's protection settings.

## Authoring order after approval to build

First establish one working Pico project, external-LED register sequence, and debugger route. Then write Chapters 0–4 around those verified files. Bring up Pi tooling and validate I²C/UART projects before writing Chapters 5–7. Establish a matching kernel build and driver lifetime design before writing Chapter 8. Write Chapter 9 against actual source from that kernel, then integrate Chapter 10. Complete appendices, rendering checks, code correspondence checks, and the public Vercel publication last.

No claim is made that the example programs or physical experiments are already implemented or tested. This plan completes the research-and-planning request; authoring and deployment remain a later task.

## Authorisation history

The plan-only phase ended when the user explicitly requested: “ok pls start and continue until the end”. Authoring, fresh page agents, independent reviews, GitHub creation and public Vercel deployment are now authorised.

Implementation decision: the primary macOS route uses the actually verified checkout-local pinned tool installer, rather than requiring an editor extension. VS Code and the official Pico extension remain optional; their managed paths must not be mixed with the book’s pinned tool paths.
