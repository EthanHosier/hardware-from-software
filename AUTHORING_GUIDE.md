# Hardware from Software: authoring and review contract

Read this entire file, PLAN.md, and your catalog.json entry. Read `/Users/hermes/Desktop/learncpp-mirror/condensed/1.4-variable-assignment-and-initialization.md` for style calibration, not facts about hardware. Work only in `/Users/hermes/Desktop/hardware-from-software`.

## Reader and writing
Experienced C++/Rust programmer, hardware newcomer. Direct Rust Book style; sufficiently detailed to learn from the page alone. Explain mechanisms, reasons, observations, failures and version qualifications. No quizzes, invented results, filler, unexplained lists of APIs, or repeated warning boxes. Every named hardware component in Chapter 0 needs what/where/why/when-use, with a labelled physical diagram. Distinguish conceptual chip internals from physical board components. All factual delicate claims require official primary references near the claim and 2–4 focused references at the end. Browse official sources; do not rely on recalled RP2040 tutorials for RP2350. Existing plan research is a starting point, not proof.

Every practical page: result/prerequisites, exact board/wiring state, needed concept, numbered actions, labelled Mac/Pi/GDB commands with working directory, complete source via mdBook include, build/run, expected observation explicitly illustrative if not measured, explanation, symptom-based troubleshooting, cleanup/next state. No fixed word quota; most substantive pages need 600–1200 useful words. Reference and short transition pages may be shorter. Scope is the catalog and PLAN, not a short outline masquerading as a lesson.

## Files and examples
Each NEW page gets a dedicated writer with `fork_turns: "none"`, fresh context, this guide first. A writer may repair its SAME page only. Own your assigned src/*.md, uniquely named SVGs under src/assets, and reviews/<ID>-writer.json. Do not modify shared code, build/catalog/config/guides, other pages, Git, or deployment. Root owns runnable examples; use their actual files and EXAMPLES.md. Request a code correction from root rather than publishing a conflicting version. Do not claim a file exists/works without inspecting it. Writers can create prose while root implements code but reports must flag pending includes; reviewer closes them before acceptance.

Use `{{#include ../examples/pico/blink_registers.cpp}}` for an entire source, normally inside a top-level, unindented cpp fence. Do not indent a full-file include inside a numbered-list fence: mdBook expands multiline includes without propagating that list indentation, which breaks the rendered code block. Introduce the file in the step, then use a top-level fence and resume numbered actions afterward. Build commands use `python3 scripts/setup_deps.py` then `python3 scripts/build_examples.py`; the setup script and EXAMPLES.md define paths. Include the shared CMakeLists when teaching project setup. Relative links point to catalog canonical filenames and local SVGs. Include files outside src are copied as downloadable files by root build. All complete code fences must match the source or use includes; snippets must say they are fragments. No fictitious exact output/latency. No full privileged commands aimed at the Mac when intended for Pi.

## Canonical hardware assignments
Pico non-wireless Pico 2, Arm Cortex-M33 single application core. External LED GP16 physical21 → 1kΩ → LED anode; cathode GND physical23. Button GP17 physical22 → switch → GND physical23; 4.7kΩ external pull-up to 3V3(OUT) physical36. For I2C: Pico GP4 physical6 SDA to Pi BCM2 physical3; Pico GP5 physical7 SCL to Pi BCM3 physical5; Pico GND physical8 to Pi GND physical6. Pi has physical board I2C pull-ups; no separate power-rail connection. Pico is target0x42, Pi controller. The Pi 4 uses its newer I2C3 controller on those same BCM2/3 contacts, `dtoverlay=i2c3,pins_2_3,baudrate=100000`, normally `/dev/i2c-3`; disable conflicting I2C1/i2c_arm routing and inspect actual adapter identity. BSC0/1 clock-stretch defects make I2C1 inappropriate for the SDK clock-stretching target. See official Raspberry Pi linux issue4884 and the frozen overlay README. No physical-runtime validation is claimed. UART: Pico GP0 physical1 TX → Pi BCM15 physical10 RX; Pico GP1 physical2 RX ← Pi BCM14 physical8 TX; GND common; 115200 8N1. Pi UART baseline uses disable-bt overlay and hciuart service stop/disable with reversible cleanup; no serial console. Pico diagnostics are USB, never protocol UART.
Pi button BCM27 physical13 → switch → GND physical14; 4.7kΩ pull-up to Pi3V3 physical17. Pi LED BCM17 physical11 →1kΩ→LED→GND physical9. GPIO v2 line offsets discovered by label/name, not fixed gpiochip index. Do not join 3V3/5V/VSYS rails between separately USB-powered boards. Remove power on both boards before changing inter-board wiring. SPI loopback uses GP18 physical24 SCK, GP19 physical25 TX, GP20 physical26 RX (SPI0); MOSI-to-MISO jumper, CS GP21 physical27 if needed. No LED pin conflict.
Optional Adafruit BME280 product2652: select 3.3V and verify header soldering, exact pin names with product docs. I2C uses GP4/GP5; SPI uses GP18/GP19/GP20 and GP21 CS. Root supplies temp-only compensation examples; explain pressure/humidity and Bosch calibration algorithm references without claiming implemented channels.

## Code readability

Keep examples readable in the rendered book: one statement per line, explicit
control-flow braces, and descriptive names that explain purpose or units. Format
C/C++ examples with clang-format 19.1.7 using the checked-in `.clang-format` files
(the kernel examples use their own Linux style). Update lesson fragments,
debugger commands, and diagram labels when identifiers change. Preserve behavior
and rerun the existing relevant builds and tests before refreshing review hashes.

## Single Linux kernel baseline

Lesson 5.2 establishes `6.12.96-hfs1-v8` from `kernel-baseline.json`, with
AF_XDP enabled. Every later Linux lesson uses this same kernel; there is no
mid-course networking kernel switch. Retain original boot files for recovery.
Build external modules against the completed native kernel output and its
Module.symvers, never an unrelated APT headers package. Kernel installation uses
private boot image/DTB/overlay paths; custom button overlays go under
`/boot/firmware/hfs-kernel/overlays/`. Check build scripts and KERNEL_BASELINE.md
before writing commands. Enabling the feature does not attach packet programs.

## Correctness focus
RP2350 pad isolation/IO bank/SIO order, atomic SET/CLR and W1C; use exact installed SDK headers and datasheet. Volatile is not synchronization. Input pull-ups and silicon E9 revision awareness. ISR/main state critical sections; no sleeps/USB printing inside ISR. Exception-frame debugger caveats: basic vs extended frames, MSP/PSP, handler prologue. UART stream framing and bounded buffer, partial I/O, termios, deadlines and errors. I2C coherent multi-byte snapshot and repeated START. Linux kernel C, descriptors/DT, IRQ context, delayed work debounce, locked state, kref/open/unbind lifetime, wake blocked readers on removal. MMIO source walkthrough, not mapping over Linux-owned controller. `mmap`/mprotect page faults only in isolated child userspace. Tools/tests are evidence not proofs. Never say hardware-confirmed without actual measurement.

## Reports and independent acceptance
Writer JSON: id, paths_read, sources (URLs), scope_audit, code_files_read, checks_performed, hardware_status, concerns, status. Separate reviewer (coordinator/root) reads every word, all included code, diagrams, commands and source claims; records reviews/<ID>-review.json with findings, repairs, scope coverage, sources checked, status. Writers repair own page; reviewer rechecks. Cannot accept solely because headings/build pass. Original-content inventories freeze ONLY after substantive review. Publish durable reports with no private data. Root integrates and performs all Git/deployment work.

Shared review layers: source/API accuracy; wiring and visual labels; progression and command reproducibility; example fence/source equality; real compile/host tests as supported; render/link/download checks; anonymous public production checks. Explicit matrix: source-reviewed / compiled / host-tested / target-compiled / physical-runtime not exercised. No invention of physical observations.

## Progressive publication (user requirement)
After EACH page passes independent review, root immediately runs `python3 scripts/publish.py`. Do not wait for the entire chapter. The accepted-only staged build requires matching Markdown, included-source and asset hashes. Unpublished forward references route to the roadmap. Check anonymous publication and append release evidence. The old private-book deployment policy does not apply to this new public site.
