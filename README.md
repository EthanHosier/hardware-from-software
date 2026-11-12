# Hardware from Software

A personal, hands-on mdBook for an experienced C++/Rust programmer learning the hardware/software boundary with a Pico 2, Raspberry Pi 4 and Raspberry Pi Debug Probe. The reading site is public at https://hardware-from-software.vercel.app; this repository is public.

The book progresses from labelled hardware and electronics-kit diagrams through GPIO registers, SWD/GDB, interrupts, I2C/SPI, UART, Linux drivers, virtual memory and an integrated project. `PLAN.md` records scope and `AUTHORING_GUIDE.md` records the fresh-writer/independent-review contract.

## Read and build

Requires mdBook 0.5.4 and Python 3.9+; `npm run build` builds the complete book after all planned lessons pass review. During authoring, `python3 scripts/build.py --partial` builds only accepted pages and `npm run publish` checks and publishes them to public Vercel. Unpublished cross-references route to the roadmap instead of broken pages. Content lives in `src/`; `catalog.json` defines order. Complete source programs live in `examples/` and are included directly in lesson code blocks.

On Apple Silicon macOS:

```sh
python3 scripts/setup_deps.py
python3 scripts/build_examples.py
python3 scripts/test_serial.py
npm run build
```

The pinned SDK/compiler/debug tools install only under ignored `.deps/`. See `EXAMPLES.md` for targets and manual commands. Linux examples and kernel modules require the appropriate Linux environment; see the book for exact commands and version qualifications.

## Evidence

`reviews/` contains writer and independent review records. `verification.json` records build, host-test and target-compile evidence plus physical-test limitations. No physical GPIO, SWD, sensor or kernel runtime result is implied by a successful compilation. GitHub Actions independently checks Linux programs and builds modules for pinned official Raspberry Pi kernel headers.

No credentials, real network identifiers, binaries, downloaded dependency trees or generated site are committed.

## Completed book

All **53 pages (44 main lessons and nine appendices)** are independently reviewed and published. The [public contents](https://hardware-from-software.vercel.app/roadmap.html) link to every page. Each page had a dedicated fresh-context writer and a separate reviewer; accepted pages were deployed individually. `reviews/authoring-status.json` and `releases.json` record that process.

For a complete checked publication, use `python3 scripts/publish.py --complete`. Physical hardware runtime remains unexercised; the verification matrix distinguishes it from software tests and target compilation.


Linux lessons share one AF_XDP-enabled Pi 4 kernel, `6.12.96-hfs1-v8`, established
in lesson 5.2 and reused for the earlier GPIO/module labs and future networking.
See `kernel-baseline.json` and `KERNEL_BASELINE.md` for source, configuration,
full-build and recovery details. Original boot files are retained as recovery
material. Physical Pi boot and peripheral runtime are separate reader checks.
