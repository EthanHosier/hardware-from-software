# Hardware from Software

This book is a laboratory guide to the boundary between code and physical hardware. It assumes you can program in C++ or Rust and understand operating-system fundamentals. It explains the electronics, embedded tools, and board details as you need them.

You will use a **Raspberry Pi Pico 2**, a **Raspberry Pi 4**, a **Raspberry Pi Debug Probe**, and the useful parts of an electronics kit. The main exercises need LEDs, resistors, buttons, a breadboard and jumper wires. A separate sensor and a logic analyser are optional.

Start with the labelled hardware tours in Chapter 0. Then work in order: build firmware, change an LED through hardware registers, inspect the store in GDB, handle a button interrupt, communicate over I²C/SPI and UART, write a Linux driver, and examine virtual memory. The final exercise follows a button event through Linux to an LED controlled by another processor.

Each practical lesson gives its wiring, commands, source files, expected observation, and troubleshooting. **Mac** means your development computer; **Pi** means a shell on the Raspberry Pi 4; **GDB** means the debugger prompt. The Pico runs the flashed firmware, not your shell commands.

The default Pico route uses C++ with the official SDK for startup and build support. Peripheral exercises expose the relevant register operations. Linux applications use C++; the kernel module uses C. No RTOS is needed.

[Download the complete examples](downloads.md), including shared headers and build scripts. Before following a physical experiment, read [what has been verified](verification.md). A successful compilation does not establish that your wiring or board behaviour has been tested.

The diagrams are labelled reference views, not photographs of your exact boards. Confirm your board model and orientation before connecting anything. Power down both boards before changing inter-board wiring; use a common signal ground and keep their separately supplied power rails separate.
