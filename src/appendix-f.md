# Appendix F — Where to go next

Choose one small extension after [verifying the complete system](10.2-verify-troubleshoot-and-explain-the-system.md). Keep its source separate and retain a known firmware image for recovery using [Appendix B](appendix-b.md). The main route remains one application core, Arm Cortex-M33, without an RTOS. These are experiment proposals, not additional validated examples; none was physically exercised during authoring. Reading this page requires no wiring changes.

## DMA: transfer ownership with the data

**Direct memory access (DMA)** moves data without a CPU instruction for each item. Start with one finite SRAM-to-SRAM transfer: copy 64 known 32-bit words into a distinct destination, then compare every word. This isolates channel setup from peripheral timing. The [SDK DMA header](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/src/rp2_common/hardware_dma/include/hardware/dma.h) defines transfer width, address increments and completion handling.

Keep both buffers alive until completion. While DMA owns the transfer, the CPU must neither change the source nor read or overwrite the destination. Configure everything before starting. Use the SDK completion operation: `dma_channel_wait_for_finish_blocking` also supplies a compiler memory barrier before subsequent buffer accesses. A delay or `volatile` buffer does not establish completion. Record the comparison result and channel error state; equality alone does not prove a speed improvement. Release the channel only after it is inactive.

## PIO: give a pin its own instruction stream

**Programmable I/O (PIO)** runs small programs in hardware state machines. A bounded first goal is to reproduce the slow GP16 LED blink from [2.1](2.1-wire-and-verify-an-external-led.md), with the same resistor and circuit, while the CPU performs unrelated work. Claim a state machine and instruction-memory space; route GP16 to that PIO instance and configure its output direction. Stop CPU GPIO writes to that pin. [SDK PIO configuration](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/src/rp2_common/hardware_pio/include/hardware/pio.h).

Derive the period from the actual system clock, divider, instruction count and delays. The divider cannot make the state machine run faster than the system clock; fractional divisors can introduce cycle jitter. FIFO starvation can stall a program using blocking pulls. Seek continued blinking while CPU work changes, then measure edges if you have [an analyser](appendix-d.md). Visible blinking establishes neither exact timing nor high-speed electrical performance. Disable the state machine before restoring the pin's previous function.

## Multicore: make ownership cross a core boundary

Start without new wiring: core 0 submits a fixed number of numbered arithmetic jobs; core 1 returns results. Keep peripherals and printing on core 0. Use bounded SDK queues, whose operations copy values and use a spinlock, with an explicit full-queue policy. Check that every submitted ID returns once with the expected result. [SDK queue implementation](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/src/common/pico_util/include/pico/util/queue.h).

Disabling interrupts on one core does not stop the other core. The single-core critical sections from [4.3](4.3-share-state-and-debounce-correctly.md) therefore need a new concurrency design; `volatile` supplies no inter-core synchronisation. Passing a pointer through a queue copies the pointer, not its pointee or ownership. Begin with values, and arrange an acknowledged shutdown before freeing queues or shared storage.

## Rust: port one observable behaviour

Port only the external LED first. The maintained [rp235x-hal examples](https://github.com/rp-rs/rp-hal/tree/main/rp235x-hal-examples) use `thumbv8m.main-none-eabihf` for Arm; [rustc documents this Cortex-M33 target](https://doc.rust-lang.org/rustc/platform-support/thumbv8m.main-none-eabi.html). Preserve their RP2350 image metadata, runtime and linker configuration, and adapt the example's GPIO25 to our GP16 circuit.

A HAL abstracts the chip; a board support package (BSP) adds board-specific configuration. Check the exact board support before selecting one: [rp-pico currently re-exports the RP2040 HAL](https://github.com/rp-rs/rp-hal-boards/blob/main/boards/rp-pico/src/lib.rs). Pin a chosen RP2350 example revision and dependencies. Seek a target-linked ELF, then separately verify boot and LED behaviour. This book supplies no compiled Rust port.

## Startup and linking: explain the existing image first

Before replacing startup, trace the SDK 2.3.1 [RP2350 linker include tree](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/src/rp2350/pico_platform/memmap_default.ld) and [Arm startup](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/src/rp2_common/pico_crt0/crt0.S). Locate the vector table, reset handler, initial stack, copied data, zeroed BSS and call into runtime initialisation and `main` in the ELF/map/disassembly.

Relate these to the RP2350 ROM's image metadata and execute-in-place (XIP) flash setup. Its [linker section comments](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/src/rp2_common/pico_standard_link/script_include/section_boot2.incl) explain why RP2040 boot2 recipes are insufficient. First produce an annotated boot path; replacing it means owning all those responsibilities, including preserving metadata needed to boot. Keep debugger loading and cold boot as separate checks.

## References

- [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf) — DMA, PIO, processor and boot mechanisms.
- [Pico SDK hardware APIs](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html) — resource configuration and limits.
- [rp-rs RP235x HAL](https://github.com/rp-rs/rp-hal/tree/main/rp235x-hal) — Rust peripheral support and development status.
- [Appendix E](appendix-e.md) — the book's actual verification evidence and limits.
