# Verification and limitations

This guide separates documentation review and software checks from experiments on physical boards. The project does not have a verified connection to your Pico, Pi, Debug Probe, or component kit.

## Software baseline

Pico examples target the non-wireless **Pico 2**, Arm Cortex-M33, with one application core. The pinned build uses Pico SDK **2.3.1**, Arm GNU Toolchain **14.2.Rel1**, CMake **3.31.6**, and Ninja **1.11.1.4**. Debug tools come from Raspberry Pi's **v2.3.1-0** tool bundle. This pins a reproducible environment rather than claiming every tool is the latest release.

Linux programs require C++20 and Linux interfaces. The kernel example uses the Linux **6.12** platform-driver API and the GPIO descriptor interface. Your installed kernel and its matching headers determine what you can load; a module built for another kernel is not a portable binary.

## What the checks establish

- A fresh writer and a separate reviewer inspect each lesson's coverage, source claims, commands, wiring labels, and included code.
- Pico compilation checks target instructions, SDK APIs and generation of firmware artifacts. It cannot establish an electrical result.
- Host protocol tests check framing, malformed input, bounds and recovery. Pseudo-terminal tests exercise serial-client stream handling; they are not UART waveform measurements.
- Linux build checks compile the actual application and kernel sources. Any runtime test is identified separately from compilation.
- Book checks verify included sources, links, downloads, diagrams and rendered pages.

The final machine-readable [verification record](verification.json) records the completed checks and exact source hashes. Review records accompany the repository.

## What you will verify on the bench

USB flashing and serial enumeration, SWD connection, LED polarity/current, physical pin connections, GPIO levels, button bounce, interrupt behaviour, bus timing, sensor identity, Linux driver runtime and end-to-end operation need the actual equipment. Their expected observations in the lessons are instructions to check, not fabricated measurements.

Follow the lesson's acceptance check before continuing. If an observation differs, use its troubleshooting section and return to the last working wiring/firmware state. Kernel runtime and teardown experiments should be performed only after the userspace GPIO baseline works and you have the documented recovery route.
