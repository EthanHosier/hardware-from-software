# Hardware from Software

A personal, hands-on mdBook for an experienced C++/Rust programmer learning the hardware/software boundary with a Pico2, Raspberry Pi4 and Raspberry Pi Debug Probe. The reading site is public at https://hardware-from-software.vercel.app; this repository is public.

The book progresses from labelled hardware and electronics-kit diagrams through GPIO registers, SWD/GDB, interrupts, I2C/SPI, UART, Linux drivers, virtual memory and an integrated project. `PLAN.md` records scope and `AUTHORING_GUIDE.md` records the fresh-writer/independent-review contract.

## Read and build

Requires mdBook0.5.4 and Python3.9+; `npm run build` builds the complete book after all planned lessons pass review. During authoring, `python3 scripts/build.py --partial` builds only accepted pages and `npm run publish` checks and publishes them to public Vercel. Unpublished cross-references route to the roadmap instead of broken pages. Content lives in `src/`; `catalog.json` defines order. Complete source programs live in `examples/` and are included directly in lesson code blocks.

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

## Current progress

Twelve of53catalog pages are published: all6Chapter0 pages, all4Chapter1 pages, and2.1–2.2. Each had a dedicated fresh-context writer and independent review. Remaining lessons are in progress. `reviews/authoring-status.json` records current per-page progress. Every accepted page must be published immediately through the checked progressive-release command.
