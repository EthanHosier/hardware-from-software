# Summary

[Start here](index.md)

[Roadmap and reading progress](roadmap.md)

[Verification and limitations](verification.md)

[Download the examples](downloads.md)


# 0. Know your hardware and your bench

- [0.1 — What you are going to build](0.1-what-you-are-going-to-build.md)
- [0.2 — A labelled tour of your Pico 2](0.2-a-labelled-tour-of-your-pico-2.md)
- [0.3 — A labelled tour of your Raspberry Pi 4](0.3-a-labelled-tour-of-your-raspberry-pi-4.md)
- [0.4 — A labelled tour of the Debug Probe and cables](0.4-a-labelled-tour-of-the-debug-probe-and-cables.md)
- [0.5 — Recognise the breadboard and kit components](0.5-recognise-the-breadboard-and-kit-components.md)
- [0.6 — Assemble the bench and understand the electrical rules](0.6-assemble-the-bench-and-understand-the-electrical-rules.md)

# 1. Build and flash your first Pico program

- [1.1 — Install a reproducible macOS toolchain](1.1-install-a-reproducible-macos-toolchain.md)
- [1.2 — Understand the smallest useful project](1.2-understand-the-smallest-useful-project.md)
- [1.3 — Flash with BOOTSEL and read output](1.3-flash-with-bootsel-and-read-output.md)
- [1.4 — Make the build–flash–observe loop repeatable](1.4-make-the-build-flash-observe-loop-repeatable.md)

# 2. Make a CPU store change a pin

- [2.1 — Wire and verify an external LED](2.1-wire-and-verify-an-external-led.md)
- [2.2 — Read the RP2350 register map](2.2-read-the-rp2350-register-map.md)
- [2.3 — Replace GPIO calls with register accesses](2.3-replace-gpio-calls-with-register-accesses.md)
- [2.4 — Follow the generated machine instructions](2.4-follow-the-generated-machine-instructions.md)

# 3. Debug the running hardware

- [3.1 — Attach the Debug Probe over SWD](3.1-attach-the-debug-probe-over-swd.md)
- [3.2 — Load the ELF and step through the LED write](3.2-load-the-elf-and-step-through-the-led-write.md)
- [3.3 — Diagnose a broken program systematically](3.3-diagnose-a-broken-program-systematically.md)

# 4. From a button to an interrupt handler

- [4.1 — Read a button by polling](4.1-read-a-button-by-polling.md)
- [4.2 — Configure and acknowledge a GPIO interrupt](4.2-configure-and-acknowledge-a-gpio-interrupt.md)
- [4.3 — Share state and debounce correctly](4.3-share-state-and-debounce-correctly.md)
- [4.4 — Inspect exception entry and return](4.4-inspect-exception-entry-and-return.md)

# 5. Bring up the Pi 4 headlessly

- [5.1 — Prepare Raspberry Pi OS and SSH](5.1-prepare-raspberry-pi-os-and-ssh.md)
- [5.2 — Verify the Linux development environment](5.2-verify-the-linux-development-environment.md)
- [5.3 — Establish GPIO identity and ownership](5.3-establish-gpio-identity-and-ownership.md)

# 6. Understand clocked buses with your existing boards

- [6.1 — I²C on the wires](6.1-i-c-on-the-wires.md)
- [6.2 — Turn the Pico into a register-based peripheral](6.2-turn-the-pico-into-a-register-based-peripheral.md)
- [6.3 — Write a Linux I²C client](6.3-write-a-linux-i-c-client.md)
- [6.4 — Compare SPI with a loopback experiment](6.4-compare-spi-with-a-loopback-experiment.md)

# 7. Connect Linux to bare-metal firmware over UART

- [7.1 — Make a UART link electrically and logically correct](7.1-make-a-uart-link-electrically-and-logically-correct.md)
- [7.2 — Configure the Pi UART and test an echo](7.2-configure-the-pi-uart-and-test-an-echo.md)
- [7.3 — Build a robust C++ serial client](7.3-build-a-robust-c-serial-client.md)
- [7.4 — Trace a command end to end](7.4-trace-a-command-end-to-end.md)

# 8. Write a Linux driver for a physical button

- [8.1 — Establish a userspace GPIO baseline](8.1-establish-a-userspace-gpio-baseline.md)
- [8.2 — Build and load the smallest external module](8.2-build-and-load-the-smallest-external-module.md)
- [8.3 — Describe the device and bind the driver](8.3-describe-the-device-and-bind-the-driver.md)
- [8.4 — Convert GPIO events into driver state](8.4-convert-gpio-events-into-driver-state.md)
- [8.5 — Expose `/dev/mybutton` to a C++ client](8.5-expose-dev-mybutton-to-a-c-client.md)
- [8.6 — Make failure and teardown behaviour explicit](8.6-make-failure-and-teardown-behaviour-explicit.md)

# 9. Virtual memory and the registers beneath the driver

- [9.1 — Observe your Linux process's mappings](9.1-observe-your-linux-process-s-mappings.md)
- [9.2 — Observe demand paging and protection faults](9.2-observe-demand-paging-and-protection-faults.md)
- [9.3 — Trace a real kernel MMIO mapping](9.3-trace-a-real-kernel-mmio-mapping.md)
- [9.4 — Explain ordering, access semantics, and ownership](9.4-explain-ordering-access-semantics-and-ownership.md)

# 10. Put the whole chain together

- [10.1 — Use a Linux device event to control Pico hardware](10.1-use-a-linux-device-event-to-control-pico-hardware.md)
- [10.2 — Verify, troubleshoot, and explain the system](10.2-verify-troubleshoot-and-explain-the-system.md)

# 11. Recognise the hardware and follow packet bytes

- [11.1 — Identify the network hardware and establish a recoverable lab](11.1-identify-network-hardware.md)
- [11.2 — Send a numbered UDP message through the normal stack](11.2-send-numbered-udp.md)
- [11.3 — Decode an Ethernet frame without trusting its layout](11.3-decode-ethernet-frame.md)

# 12. Follow the Ethernet driver into the chip

- [12.1 — Connect Device Tree, driver probe, MMIO and the PHY](12.1-trace-network-driver-probe.md)
- [12.2 — Follow a transmit buffer through DMA and completion](12.2-trace-network-transmit-dma.md)
- [12.3 — Follow receive DMA, interrupts and NAPI into a socket](12.3-trace-network-receive-napi.md)

# 13. Move packet processing toward userspace

- [13.1 — Receive through a shared packet ring](13.1-receive-packet-ring.md)
- [13.2 — Run a selective generic XDP program](13.2-selective-generic-xdp.md)
- [13.3 — Verify AF_XDP availability and understand its memory model](13.3-verify-af-xdp-memory.md)
- [13.4 — Redirect a lab flow into AF_XDP receive buffers](13.4-af-xdp-receive.md)
- [13.5 — Transmit replies and reclaim buffers correctly](13.5-af-xdp-transmit-completion.md)
- [13.6 — Compare paths and explain full kernel bypass](13.6-compare-network-paths.md)

# Appendices

- [A — Pinouts and circuits](appendix-a.md)
- [B — Build and recovery reference](appendix-b.md)
- [C.1 — Meet an optional BME280](appendix-c-1.md)
- [C.2 — Read a BME280 over I²C](appendix-c-2.md)
- [C.3 — Convert raw sensor data](appendix-c-3.md)
- [C.4 — Read the same chip over SPI](appendix-c-4.md)
- [D — Optional logic analyser](appendix-d.md)
- [E — References and glossary](appendix-e.md)
- [F — Where to go next](appendix-f.md)
