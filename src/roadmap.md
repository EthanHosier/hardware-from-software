# Roadmap and reading progress

All lessons and appendices below are published and independently reviewed.

<a id="lesson-0-1"></a>
## 0.1 — What you are going to build

[Read this lesson](0.1-what-you-are-going-to-build.md).

Map the seven original exercises onto this book. Contrast a Linux process, kernel driver, bare-metal firmware, processor, peripheral controller, and physical component. Show the eventual Mac → SSH → Pi → UART → Pico → LED chain, with photographs or accurate board silhouettes identifying the actual devices.

<a id="lesson-0-2"></a>
## 0.2 — A labelled tour of your Pico 2

[Read this lesson](0.2-a-labelled-tour-of-your-pico-2.md).

Provide an annotated top view of the actual baseline board and a separate internal block diagram. Label the RP2350 microcontroller package, external flash chip, USB connector, BOOTSEL button, onboard LED, GPIO headers, power/ground pins, SWD connector, and the relevant clock/power circuitry. Explain processor cores, SRAM, and peripheral controllers as parts inside the RP2350 rather than separate visible chips. Distinguish flash from SRAM, the chip from the board, and a physical pin from its selectable peripheral functions. Explain what each labelled item does and when we use it. Identify board orientation and Pico 2 versus Pico 2 W differences explicitly.

<a id="lesson-0-3"></a>
## 0.3 — A labelled tour of your Raspberry Pi 4

[Read this lesson](0.3-a-labelled-tour-of-your-raspberry-pi-4.md).

Provide annotated top and underside views. Label the BCM2711 system-on-chip package, RAM package, USB controller, Ethernet controller/connector, USB ports, USB-C power input, micro-HDMI ports, microSD slot, 40-pin GPIO header and pin-1 marker, wireless section, and visible display/camera connectors. Explain SoC versus CPU versus board, RAM versus microSD storage, and controllers versus physical connectors. Cover supporting power/clock circuitry at a useful overview level, without trying to name every passive component. State which parts are used in our labs and which can be left alone. Account for a heatsink or case obscuring parts without requiring removal.

<a id="lesson-0-4"></a>
## 0.4 — A labelled tour of the Debug Probe and cables

[Read this lesson](0.4-a-labelled-tour-of-the-debug-probe-and-cables.md).

Annotate the probe's USB, SWD, and UART connections and their signal names; explain the probe's own controller separately from the target Pico. Show the actual connector orientation and the path each cable serves. Inventory data-capable micro-USB cables, USB-C adapters/hub, supplied SWD cable, jumper wire ends, power supply, and SD adapter. Distinguish power, data, debug, and serial connections, including the fact that the SWD connector has no target-power pin.

<a id="lesson-0-5"></a>
## 0.5 — Recognise the breadboard and kit components

[Read this lesson](0.5-recognise-the-breadboard-and-kit-components.md).

Use labelled diagrams of the breadboard's hidden contact strips, centre gap, and possibly split power rails; show how the Pico straddles the gap. Identify resistors and their values, LED anode/cathode and polarity markings, four-leg button internal contact pairs, and jumper wire types. Explain what each component contributes to our circuits. Identify these against the actual kit rather than inventing included parts.

<a id="lesson-0-6"></a>
## 0.6 — Assemble the bench and understand the electrical rules

[Read this lesson](0.6-assemble-the-bench-and-understand-the-electrical-rules.md).

Consolidate the physical inventory and power/data/debug connection diagram. Explain ground/reference voltage, 3.3V logic, LED current limiting and resistor selection, pull resistors, floating inputs, and separate USB supplies. Explain why signal-ground wires are shared but power outputs are not joined. Establish consistent top-view orientation, GPIO names versus physical pin numbering, and a habit of powering down before changing wiring.

<a id="lesson-1-1"></a>
## 1.1 — Install a reproducible macOS toolchain

[Read this lesson](1.1-install-a-reproducible-macos-toolchain.md).

Install the checkout-local pinned official SDK/Arm/tool bundle with scripts/setup_deps.py on Apple Silicon; record exact compiler/build/debug paths and versions. Official Pico VSCode extension is an optional separate managed-tool route, not required or mixed with .deps. Explain host versus target binaries and verification commands.

<a id="lesson-1-2"></a>
## 1.2 — Understand the smallest useful project

[Read this lesson](1.2-understand-the-smallest-useful-project.md).

Complete CMake and C++ files; explicit Pico 2/Arm target, debug build, SDK import, SDK initialisation, and USB serial output. Explain compilation, linking, ELF, UF2, flash, SRAM, and the SDK's startup contribution.

<a id="lesson-1-3"></a>
## 1.3 — Flash with BOOTSEL and read output

[Read this lesson](1.3-flash-with-bootsel-and-read-output.md).

Enter BOOTSEL, copy the UF2, observe the RP2350 mass-storage device disappear, locate the serial port, and read a versioned heartbeat. Account for macOS serial device naming and charge-only cables.

<a id="lesson-1-4"></a>
## 1.4 — Make the build–flash–observe loop repeatable

[Read this lesson](1.4-make-the-build-flash-observe-loop-repeatable.md).

Change a heartbeat value, rebuild, flash, verify the new firmware, and recover from a program that no longer starts its USB interface. Establish stable example names and a version report.

<a id="lesson-2-1"></a>
## 2.1 — Wire and verify an external LED

[Read this lesson](2.1-wire-and-verify-an-external-led.md).

Use one conservative resistor value from the kit and a specified external GPIO. Provide the complete pin-to-resistor-to-LED-to-ground circuit. Briefly use the SDK GPIO API to establish a known-good circuit.

<a id="lesson-2-2"></a>
## 2.2 — Read the RP2350 register map

[Read this lesson](2.2-read-the-rp2350-register-map.md).

Derive the chosen pin's pad control, function selection, SIO output enable, and output registers from the RP2350 datasheet. Explain address, register width, access type, masks, reset state, pad isolation, and read-modify-write hazards.

<a id="lesson-2-3"></a>
## 2.3 — Replace GPIO calls with register accesses

[Read this lesson](2.3-replace-gpio-calls-with-register-accesses.md).

Keep SDK startup, replace GPIO setup/output with documented register operations, and use SET/CLR aliases where appropriate. Introduce volatile access and its limits; distinguish peripheral atomic operations from C++ inter-thread atomics.

<a id="lesson-2-4"></a>
## 2.4 — Follow the generated machine instructions

[Read this lesson](2.4-follow-the-generated-machine-instructions.md).

Compare source, disassembly, address calculation, and the store to a peripheral register. Explain that optimisation, bus transactions, pad configuration, and physical timing are separate layers.

<a id="lesson-3-1"></a>
## 3.1 — Attach the Debug Probe over SWD

[Read this lesson](3.1-attach-the-debug-probe-over-swd.md).

Separate SWD and probe UART connectors, verify SWDIO/SWCLK/GND, power the target separately, and run OpenOCD with RP2350 support and the correct target configuration. Account for probe firmware compatibility.

<a id="lesson-3-2"></a>
## 3.2 — Load the ELF and step through the LED write

[Read this lesson](3.2-load-the-elf-and-step-through-the-led-write.md).

Connect Arm GDB, reset/halt, load the debug ELF, break at a known point, inspect variables/registers, and step across the output store. Explain stopped-CPU effects and stale source/binary mistakes.

<a id="lesson-3-3"></a>
## 3.3 — Diagnose a broken program systematically

[Read this lesson](3.3-diagnose-a-broken-program-systematically.md).

Use supplied reversible faults: wrong mask, missing output enable, wrong mux, and a bounded loop. Inspect stack, PC, peripheral state, and disassembly. Explain when reading a register can have side effects.

<a id="lesson-4-1"></a>
## 4.1 — Read a button by polling

[Read this lesson](4.1-read-a-button-by-polling.md).

Use an active-low button and an explicitly specified external pull-up; explain contact pairs, input enable, electrical versus logical state, and the relevant silicon erratum/revision qualification. Poll an input register.

<a id="lesson-4-2"></a>
## 4.2 — Configure and acknowledge a GPIO interrupt

[Read this lesson](4.2-configure-and-acknowledge-a-gpio-interrupt.md).

Move from the SDK callback baseline to the relevant GPIO interrupt enable/status/acknowledge path and CPU IRQ enable. Explain edge versus level, NVIC, vector entry, and write-one-to-clear semantics.

<a id="lesson-4-3"></a>
## 4.3 — Share state and debounce correctly

[Read this lesson](4.3-share-state-and-debounce-correctly.md).

Keep the ISR short; no sleeping or USB logging there. Use a documented single-core handoff with appropriate critical sections or proven lock-free operations, and a timed debounce state machine. Explain why volatile alone is not synchronisation.

<a id="lesson-4-4"></a>
## 4.4 — Inspect exception entry and return

[Read this lesson](4.4-inspect-exception-entry-and-return.md).

Break in the handler, inspect the Cortex-M33 exception frame and return path, observe pending interrupts, and explain how halted execution distorts timing. Contrast polling, interrupt notification, and optional wait-for-interrupt behaviour.

<a id="lesson-5-1"></a>
## 5.1 — Prepare Raspberry Pi OS and SSH

[Read this lesson](5.1-prepare-raspberry-pi-os-and-ssh.md).

Support checking a preinstalled card and, when necessary, imaging a selected 64-bit Raspberry Pi OS Lite release. Configure user, hostname, network, SSH, and key authentication; use the actual Mac network route. Explain local-network discovery and the fallback to an IP address.

<a id="lesson-5-2"></a>
## 5.2 — Verify the Linux development environment

[Read this lesson](5.2-verify-the-linux-development-environment.md).

Identify the64-bit Pi environment; build/install the one pinned AF_XDP-enabled6.12.96-hfs1-v8 kernel before Linux labs, retaining recovery files; verify release/config/socket support, run nativehello and retain exact completed module buildtree.

<a id="lesson-5-3"></a>
## 5.3 — Establish GPIO identity and ownership

[Read this lesson](5.3-establish-gpio-identity-and-ownership.md).

Distinguish physical header pins, BCM names, gpiochip line offsets, and requested lines. Use current GPIO character-device tooling to inspect the actual controller instead of assuming a gpiochip number. Introduce line ownership and pin multiplexing.

<a id="lesson-6-1"></a>
## 6.1 — I²C on the wires

[Read this lesson](6.1-i-c-on-the-wires.md).

Explain open-drain signalling, pull-ups, START/STOP, 7-bit addresses, R/W bit, ACK/NACK, register pointers, and repeated START. Wire Pi controller to Pico target using the documented Pi pull-ups and 3.3V signalling; separate powering and common ground.

<a id="lesson-6-2"></a>
## 6.2 — Turn the Pico into a register-based peripheral

[Read this lesson](6.2-turn-the-pico-into-a-register-based-peripheral.md).

Implement a small versioned I²C register interface inspired by the official memory-target example. Expose identity, LED control, button count, and a coherent snapshot of the internal temperature ADC reading. Keep bus callbacks bounded and handle repeated START and multi-byte state correctly.

<a id="lesson-6-3"></a>
## 6.3 — Write a Linux I²C client

[Read this lesson](6.3-write-a-linux-i-c-client.md).

Enable the intended Pi I²C bus, identify `/dev/i2c-*`, and use a complete C++ client with `I2C_RDWR` for combined transactions. Read identity/data, control the LED, and diagnose wrong addresses, NACKs, and partial/failed transfers. Distinguish an approximate chip temperature from ambient measurement.

<a id="lesson-6-4"></a>
## 6.4 — Compare SPI with a loopback experiment

[Read this lesson](6.4-compare-spi-with-a-loopback-experiment.md).

Use the Pico SPI controller in a simple loopback exercise, initially with documented loopback support or an explicit MOSI-to-MISO wire. Explain SCK, MOSI/MISO, chip select, CPOL/CPHA, full duplex, and why echo success does not validate a real peripheral's timing. Detach any earlier wiring that conflicts.

<a id="lesson-7-1"></a>
## 7.1 — Make a UART link electrically and logically correct

[Read this lesson](7.1-make-a-uart-link-electrically-and-logically-correct.md).

Cross Pi TX to Pico RX and vice versa, share ground, and use 3.3V logic. Explain baud rate, start/stop bits, idle state, and the distinction between UART, USB serial, and RS-232. Establish a fixed 115200/8-N-1 baseline.

<a id="lesson-7-2"></a>
## 7.2 — Configure the Pi UART and test an echo

[Read this lesson](7.2-configure-the-pi-uart-and-test-an-echo.md).

Disable the serial login console while enabling UART hardware. Resolve Pi 4 Bluetooth/PL011 versus mini-UART routing explicitly, inspect `/dev/serial0`, and verify the actual port. Keep Pico diagnostic output separate from protocol traffic.

<a id="lesson-7-3"></a>
## 7.3 — Build a robust C++ serial client

[Read this lesson](7.3-build-a-robust-c-serial-client.md).

Use termios, bounded buffers, timeouts/poll, partial reads/writes, interruption handling, and clean close. Implement a small newline-framed protocol with request identifiers, bounded lengths, explicit errors, and commands for LED, button count, and temperature.

<a id="lesson-7-4"></a>
## 7.4 — Trace a command end to end

[Read this lesson](7.4-trace-a-command-end-to-end.md).

Follow an SSH-launched command through Linux write/read, the UART controller, Pico receive/parsing, the register store, and acknowledgement. Compare USB logs, optional analyser capture, and debugger evidence; distinguish throughput from latency.

<a id="lesson-8-1"></a>
## 8.1 — Establish a userspace GPIO baseline

[Read this lesson](8.1-establish-a-userspace-gpio-baseline.md).

Rewire a button and LED to dedicated Pi header pins, confirm availability, and use a C++ client against the GPIO character-device v2 API to request lines and receive edges. Explain why an educational custom driver is not required for every GPIO application.

<a id="lesson-8-2"></a>
## 8.2 — Build and load the smallest external module

[Read this lesson](8.2-build-and-load-the-smallest-external-module.md).

Install headers matching the running kernel, use kbuild, and build a minimal C module with load/unload logging. Explain module metadata, kernel/user address spaces, and recovery if the module or headers mismatch. Reuse the completed6.12.96-hfs1-v8 course-kernel output and Module.symvers from5.2; no distribution-header substitution.

<a id="lesson-8-3"></a>
## 8.3 — Describe the device and bind the driver

[Read this lesson](8.3-describe-the-device-and-bind-the-driver.md).

Provide a complete Device Tree overlay and matching platform driver. Use a custom compatible string, named `*-gpios` properties, descriptor acquisition, explicit pin configuration, and error unwinding. Explain probe/binding and how to disable the overlay.

<a id="lesson-8-4"></a>
## 8.4 — Convert GPIO events into driver state

[Read this lesson](8.4-convert-gpio-events-into-driver-state.md).

Map the button descriptor to an IRQ, use an appropriate threaded handler/deferred debounce design, and update protected event state. Explain hard IRQ versus process/thread context and which operations may sleep.

<a id="lesson-8-5"></a>
## 8.5 — Expose `/dev/mybutton` to a C++ client

[Read this lesson](8.5-expose-dev-mybutton-to-a-c-client.md).

Use a misc device and deliberately small text-record ABI. Define blocking/nonblocking reads, poll readiness, short-buffer behaviour, errors, and permissions. Use a monotonic event counter with per-open last-seen state; explicitly document that this is a latest-state/counter interface, not an unlimited event queue.

<a id="lesson-8-6"></a>
## 8.6 — Make failure and teardown behaviour explicit

[Read this lesson](8.6-make-failure-and-teardown-behaviour-explicit.md).

Test concurrent readers under the defined counter semantics, repeated load/unload, signal interruption, a held button, bounce, and open descriptors. Define an explicit offline state and a reference-counted userspace-facing object so unbind wakes waiters and returns a documented device-gone error without freed-memory access. Compare with standard gpio-keys/input drivers.

<a id="lesson-9-1"></a>
## 9.1 — Observe your Linux process's mappings

[Read this lesson](9.1-observe-your-linux-process-s-mappings.md).

Inspect `/proc/self/maps`, allocate/map ordinary memory, query page size, and compare virtual addresses in separate runs. Explain why pointer values do not reveal physical addresses and why an MCU MMIO address is not a usable Linux userspace pointer.

<a id="lesson-9-2"></a>
## 9.2 — Observe demand paging and protection faults

[Read this lesson](9.2-observe-demand-paging-and-protection-faults.md).

Use a bounded userspace program to touch mapped anonymous pages and compare minor-fault counts; use a child process and `mprotect` for a controlled protection fault. Explain why observed counts vary and why normal demand faults differ from invalid accesses.

<a id="lesson-9-3"></a>
## 9.3 — Trace a real kernel MMIO mapping

[Read this lesson](9.3-trace-a-real-kernel-mmio-mapping.md).

Inspect the live Device Tree and resource descriptions, then follow the matching Pi kernel GPIO/pinctrl source from resource lookup/mapping to register access. Explain address translation (`reg`/`ranges`), `ioremap` and managed mapping helpers, `__iomem`, device attributes, and `readl`/`writel`. Use source matching the selected kernel, not a random tutorial's physical addresses.

<a id="lesson-9-4"></a>
## 9.4 — Explain ordering, access semantics, and ownership

[Read this lesson](9.4-explain-ordering-access-semantics-and-ownership.md).

Compare Pico volatile pointers with Linux MMIO accessors, memory barriers, write-one-to-clear registers, SET/CLR aliases, resource ownership, and ordinary RAM. Walk through a minimal resource-owning mapping example as annotated source, not a module that maps over the active GPIO controller.

<a id="lesson-10-1"></a>
## 10.1 — Use a Linux device event to control Pico hardware

[Read this lesson](10.1-use-a-linux-device-event-to-control-pico-hardware.md).

Reuse `/dev/mybutton` and the robust UART client. A physical Pi button event wakes the C++ process, which sends an identified LED command to the Pico and verifies its reply. Show separate Pi/Pico LEDs and all wiring on one consolidated diagram.

<a id="lesson-10-2"></a>
## 10.2 — Verify, troubleshoot, and explain the system

[Read this lesson](10.2-verify-troubleshoot-and-explain-the-system.md).

Use a repeatable end-to-end checklist: restart the client, reset the Pico, remove/reconnect the data link with boards powered down for wiring changes, reject malformed requests, and handle the driver going offline. Revisit what each layer provides and identify the next experiment by interest.

<a id="lesson-11-1"></a>
## 11.1 — Identify the network hardware and establish a recoverable lab

[Read this lesson](11.1-identify-network-hardware.md).

Label MAC/PHY/RJ45/MDIO; discoverPi4GENETinterface; separateWiFimanagementandwiredtest; safeprivatevethnamespace setup/cleanup andboundedconnectivity checks.

<a id="lesson-11-2"></a>
## 11.2 — Send a numbered UDP message through the normal stack

[Read this lesson](11.2-send-numbered-udp.md).

CompletefiniteUDPendpoint+portableMacsender,version/magic/sequencevalidation,deadlines,ARPandstackdiagram,filteredcapturesandboundedfailurecases.

<a id="lesson-11-3"></a>
## 11.3 — Decode an Ethernet frame without trusting its layout

[Read this lesson](11.3-decode-ethernet-frame.md).

CompleteboundedAF_PACKETreceiverwithsharedcheckedARP/IPv4/UDPdecoder,endiannessandbounds,unsupportedVLAN/IPv6/fragments,offload/FCS/preamblelimits,fixturetests.

<a id="lesson-12-1"></a>
## 12.1 — Connect Device Tree, driver probe, MMIO and the PHY

[Read this lesson](12.1-trace-network-driver-probe.md).

TraceactualGENETsource/resourcesandPHY/MDIO; distinguishexportedregisterresources,softwarebuffersandMMIOdescriptors;readonlyidentity/sourcecheckswithmatchedversion.

<a id="lesson-12-2"></a>
## 12.2 — Follow a transmit buffer through DMA and completion

[Read this lesson](12.2-trace-network-transmit-dma.md).

Tracebcmgenet_xmit/skb/DMAmap/descriptors/producer/completion/unmap,errorlifetimes,addressspacesandownership;boundedUDPobservation,offloadsandlimits.

<a id="lesson-12-3"></a>
## 12.3 — Follow receive DMA, interrupts and NAPI into a socket

[Read this lesson](12.3-trace-network-receive-napi.md).

TraceGENETRXbufferrefill/unmap/IRQ/NAPIbudget/GRO/stackhandoff;read-onlycounterandIRQbefore/afterexperiment,correlationlimitsandoptionalavailabletracepoints.

<a id="lesson-13-1"></a>
## 13.1 — Receive through a shared packet ring

[Read this lesson](13.1-receive-packet-ring.md).

CompleteboundedTPACKET_V3RXreader,block/frameboundsandownership/acquire-release,poll/deadline/dropstats;shareddecoder;notNIChardwareDMAorzerocopy.

<a id="lesson-13-2"></a>
## 13.2 — Run a selective generic XDP program

[Read this lesson](13.2-selective-generic-xdp.md).

CompleteBPFprogram+ownedloader,verifier/pass/count/dropselectedmarkedUDPonly,SKBexplicit,preexistingattachmentrefusalandownedcleanup,namespacefirstandphysicalmanagementguard.

<a id="lesson-13-3"></a>
## 13.3 — Verify AF_XDP availability and understand its memory model

[Read this lesson](13.3-verify-af-xdp-memory.md).

Reuse5.2singlekernelrelease/configandminimalsocketchecks;UMEM/FILL/RX/TX/COMPLETIONdiagram;generic/nativeversuscopy/zerocopy;libxdp/libbpftoolchainprep,nosecondkernel.

<a id="lesson-13-4"></a>
## 13.4 — Redirect a lab flow into AF_XDP receive buffers

[Read this lesson](13.4-af-xdp-receive.md).

Completegeneric/copyAF_XDPreceiverwithboundedUMEM/FILL/RX,checkedoneRXqueue0topology,selectivemapredirectreadyfallbackPASS,counters/sequenceaccountingandcleanup.

<a id="lesson-13-5"></a>
## 13.5 — Transmit replies and reclaim buffers correctly

[Read this lesson](13.5-af-xdp-transmit-completion.md).

CompleteboundedAF_XDPresponderwithTX/COMPLETIONownership,repliesforvalidlabUDPonly,MAC/IP/portswapandchecksums,noin-flightreuse,ringavailability/deadline/drain/cleanup.

<a id="lesson-13-6"></a>
## 13.6 — Compare paths and explain full kernel bypass

[Read this lesson](13.6-compare-network-paths.md).

FairfiniteUDPversusAF_XDPechoandseparatecapturecomparison,CPU/loss/RTTmode/pathmatrix,pollingtradeoffs,DPDKdirectPMDversusAF_XDPPMDconcepts,normalcoursekernelcleanup,nohardwarezerocopypromise.

<a id="lesson-a"></a>
## A — Pinouts and circuits

[Read this lesson](appendix-a.md).

Pin maps and wiring states; component diagram index and colour conventions.

<a id="lesson-b"></a>
## B — Build and recovery reference

[Read this lesson](appendix-b.md).

Complete terminal build route and recovery commands; scripts/setup-deps.py and build commands. No invented tool paths.

<a id="lesson-c-1"></a>
## C.1 — Meet an optional BME280

[Read this lesson](appendix-c-1.md).

Adafruit BME280 breakout product 2652 as optional, soldering/headers explicit; label identity and datasheet; Bosch register map.

<a id="lesson-c-2"></a>
## C.2 — Read a BME280 over I²C

[Read this lesson](appendix-c-2.md).

Exact Pico wiring and raw chip-ID/calibration/data registers using bme280_i2c target. Complete source include. Explain repeated START.

<a id="lesson-c-3"></a>
## C.3 — Convert raw sensor data

[Read this lesson](appendix-c-3.md).

Read calibration, compensation temperature and pressure/humidity where implemented; use actual source, measurement limitations. If temperature only, state scope and explain other channels, do not claim more.

<a id="lesson-c-4"></a>
## C.4 — Read the same chip over SPI

[Read this lesson](appendix-c-4.md).

Exact wiring and bme280_spi source; protocol comparison with I2C and chip identity checks.

<a id="lesson-d"></a>
## D — Optional logic analyser

[Read this lesson](appendix-d.md).

Device-specific worked path for a Saleae Logic 8 with official Logic 2 docs on macOS; no purchasing necessary. Captures described, never fabricated. Wiring and sample-rate rationale; other devices not claimed compatible.

<a id="lesson-e"></a>
## E — References and glossary

[Read this lesson](appendix-e.md).

Primary sources organised by topic, glossary, software and hardware verification matrix from verification.json; honest status.

<a id="lesson-f"></a>
## F — Where to go next

[Read this lesson](appendix-f.md).

Short useful next steps for DMA PIO multicore Rust startup/linker; no pretend lessons.

