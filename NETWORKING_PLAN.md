# Networking extension: Chapters 11–13

Status: the user authorised the single-kernel migration and completion of all networking lessons. All 12 lessons are written and independently reviewed; final publication checks are recorded in reviews/. The baseline is established in Chapter 5 and reused here.

## Goal and shape

Add three chapters with **12 lessons** after Chapter 10. Start with the Pi 4's actual Ethernet hardware, follow a normal UDP exchange into its Linux driver, then progressively change where packet processing happens. Preserve the book's diagrams, complete readable examples, source-grounded explanations and independent acceptance method.

The central question is: **which component owns these bytes now, what makes them visible to the next component, and which software layers did this packet traverse?** Carry one small, numbered UDP message through the entire extension. TCP gets a comparison of responsibilities (connection state, retransmission, ordering), not an additional implementation project.

Pico 2 + W5500 is a later extension, outside this approval. No networking module or replacement board is required here.

## Hardware, topology and one course kernel

- Existing Mac, Pi 4, its power supply, microSD and normal imaging/access equipment. For physical Ethernet labs: one Ethernet cable and a reachable LAN port on the existing router/switch. Mac may stay on Wi-Fi. No Ethernet adapter for the Mac is assumed.
- Diagram two paths: Wi-Fi SSH management to the Pi, and test packets reaching the Pi's wired interface through the LAN. Check route, interface address, carrier, driver and actual ingress before proceeding; interface names are discovered rather than hardcoded. AP/client isolation and different subnets can prevent this topology from working.
- If the LAN does not permit that path, the same Linux exercises run over a private veth pair and network namespace on the Pi. No bridge, NAT, default-route change, DHCP reconfiguration or internet connectivity is needed for that lab. Mark this as a software link, with no physical PHY/DMA evidence.
- Start active filter/redirection experiments in the namespace lab. Physical repetition uses Wi-Fi management verified in a separate SSH session. An Ethernet-only SSH setup retains the namespace route for active tests.
- Pico and inter-board bus/debug wires remain disconnected. Existing GPIO circuits need no changes. Stop the Chapter 10 client; the networking exercises do not require its custom GPIO module to remain loaded.
- One course kernel, established once in lesson 5.2: **`6.12.96-hfs1-v8`**, built from frozen Raspberry Pi source commit `ae161617d7a4552fa91d03626b8d9f3696d17481` and the official configuration with `CONFIG_XDP_SOCKETS=y`. It serves all GPIO, I²C, UART, module, memory and networking lessons. Normal networking is unchanged until a program is explicitly attached and redirects packets.
- Chapter 5 performs the source build and installation, including the matching completed module build tree and symbol versions. Original boot files remain recovery material, not a second teaching profile. Later networking lessons neither switch kernels nor ask readers to rebuild it again. A distinct local release prevents module mixing and package updates from replacing the selected course image silently.
- A separately specified Linux VM/CI kernel with AF_XDP enabled can run the namespace software tests; a Mac alone cannot execute Linux BPF/AF_XDP. This alternative does not validate Pi boot, GENET, PHY or DMA. It is an explicit substitute software environment, not an automatic fallback claimed to repair the Pi's missing kernel feature.

## Research decisions that constrain the lessons

1. The pinned GENET driver's operations lack native XDP and AF_XDP zero-copy hooks. The Pi route therefore explicitly uses **generic/SKB XDP** and **AF_XDP copy mode**. A successful generic attachment does not establish native support. Recheck driver/source/config on a different kernel. [Pinned GENET source](https://raw.githubusercontent.com/raspberrypi/linux/ae161617d7a4552fa91d03626b8d9f3696d17481/drivers/net/ethernet/broadcom/genet/bcmgenet.c).
2. The original distribution configuration disables **CONFIG_XDP_SOCKETS**; the single course kernel enables it during initial setup. BPF, BPF_SYSCALL, BPF_JIT, PACKET, NET_NS and VETH remain available. Check the selected course release, booted configuration and runtime socket support rather than assuming that an unrelated image has this feature. Reference configuration and AF_XDP Kconfig are linked below.
3. Define three separate axes: ordinary protocol-stack processing, packet-data copies, and kernel/driver involvement. AF_XDP copy mode skips normal protocol processing for redirected packets but retains kernel work and a copy. A veth demonstration cannot prove hardware zero-copy. [Linux 6.12 AF_XDP](https://docs.kernel.org/6.12/networking/af_xdp.html).
4. PACKET_MMAP teaches shared-memory ownership and batching; mapped packet rings are not the Ethernet controller's DMA descriptors. [Linux 6.12 packet rings](https://docs.kernel.org/6.12/networking/packet_mmap.html).
5. AF_XDP application helpers come from modern **libxdp**; libbpf handles the BPF side. Pin mutually compatible versions and a clang build with the BPF target during implementation. Use maintained ring helpers rather than inventing synchronization. [Upstream libxdp](https://github.com/xdp-project/xdp-tools/blob/main/lib/libxdp/README.org).

## Chapter 11 — Recognise the hardware and follow packet bytes (3 lessons)

### 11.1 Identify the network hardware and establish a recoverable lab

**Result:** identify what the Pi's networking components do and establish one verified traffic path.

- Label the SoC's Ethernet MAC, separate PHY, magnetics/RJ45 assembly, Ethernet cable and Wi-Fi section. Explain MAC versus MAC address, PHY, MDIO management versus packet-data connection, link negotiation and why a socket is not a hardware port.
- Give a physical board view and a separate conceptual CPU/RAM/MAC/PHY diagram. Explain clock-domain and parallel-to-serial boundaries conceptually without promising access to analogue Ethernet waveforms or undocumented register details.
- Discover the wired interface, driver, addresses, route, link speed/duplex and driver bus identity using read-only tools. Avoid confusing Pi 4 Ethernet with its separate USB controller or Pi 3's networking architecture.
- Record management connectivity and original state. Establish the private namespace/veth lab with explicit create/show/cleanup actions and collision checks for names, addresses and existing host routes. Verify both namespace peers without altering the default route.

**Evidence:** an interface/driver/topology record and bounded connectivity checks using existing tools (for example, a finite ping); the numbered application packet begins in 11.2. Software-only and physical routes are identified separately.

### 11.2 Send a numbered UDP message through the normal stack

**Result:** a complete C++ UDP endpoint and a small portable Mac sender exchange finite, numbered messages.

- Explain bind, address/port, socket receive buffers, datagram boundaries, network byte order, payload length and UDP's lack of delivery/order guarantees.
- Use a fixed version/magic/sequence payload, bounded count, short maximum size and absolute deadlines. Reject invalid length/version and unexpected replies. No unlimited echo server or broad traffic generator.
- Draw transmit and receive stack paths, including neighbour lookup, Ethernet framing and where ARP fits. Contrast local loopback with actual wired traffic.
- Observe the selected test flow with filtered tcpdump; correlate payload IDs with application observations. Explain local versus remote timestamps; do not infer one-way latency from unsynchronized clocks.
- Reuse the endpoint as the reference receiver for later pass/drop/redirect tests.

**Evidence:** matching sequence/payload records, captured example bytes and an explicit timeout/malformed-message outcome. Printed transcripts remain illustrative until measured.

### 11.3 Decode an Ethernet frame without trusting its layout

**Result:** a complete bounded AF_PACKET receiver decodes only the supported lab frames and matches an independent packet capture.

- Annotate MAC addresses/EtherType, ARP fields, IPv4 header length/total length, UDP ports/length and payload. Explain endianness, alignment and why casting arbitrary bytes to C++ structures is insufficient.
- Reusable checked byte decoder; accept ordinary untagged IPv4/UDP and relevant ARP, explicitly classify unsupported VLAN/IPv6/fragments and truncated frames. Do not implement IP reassembly or a full network stack.
- Explain raw sockets' privilege requirements and how received frames differ from physical signalling. Account for padding, typically absent FCS/preamble and offload/capture artefacts, including apparently bad outgoing checksums. Observe offload settings; do not change them silently.
- Compare packet capture with normal socket delivery: raw access does not mean that Linux is absent from the data path.

**Evidence:** deterministic fixture tests covering valid and malformed lengths, normal/sanitized parser execution, and a reader procedure correlating captured fields with the UDP endpoint.

## Chapter 12 — Follow the Pi's Ethernet driver into the chip (3 lessons)

### 12.1 Connect Device Tree, driver probe, MMIO and the PHY

**Result:** a concrete source map from the discovered Ethernet device to its controller and PHY setup.

- Match the running device/driver to the selected source revision. Follow Device Tree resources and GENET probe, mapped register accessors, net_device registration, PHY discovery and MDIO/link-state handling.
- Distinguish controller registers, software objects, payload memory and descriptor locations using the actual GENET implementation. Its descriptor access uses MMIO helpers; do not substitute a generic diagram that claims every descriptor is ordinary coherent RAM.
- Reuse Chapter 9's ownership/address-space concepts. Read exported identity, resource and link information; no /dev/mem writes, driver unbind or second driver over the live controller.

**Evidence:** an annotated driver/source/resource diagram and exact source landmarks, qualified by release.

### 12.2 Follow a transmit buffer through DMA and completion

**Result:** explain the lifetime of one transmitted packet from socket submission to driver reclamation.

- Trace `bcmgenet_xmit`, skb head/fragments, DMA mapping/error handling, descriptor addresses/status, producer index publication, completion and unmapping/freeing.
- Distinguish user virtual, kernel virtual, physical/resource and DMA addresses; explain DMA direction, visibility, lifetime and why volatile or sleeping does not transfer ownership.
- Label where headers/checksums can be added or offloaded. A successful send or TX completion does not prove peer receipt.
- Work through a labelled buffer/descriptor ownership timeline and a finite-send observation using the previous endpoint and relevant counters.

**Evidence:** source-grounded ownership timeline, a checked description of failure/reclaim paths, and bounded reader measurements with no physical address invention.

### 12.3 Follow receive DMA, interrupts and NAPI into a socket

**Result:** connect incoming data, RX refill, IRQ work, NAPI processing and application delivery.

- Trace buffer allocation/map, completed descriptors, buffer replacement/unmap, GENET interrupt scheduling, RX poll budget and handoff through `napi_gro_receive` into the protocol stack.
- Explain hard IRQ versus softirq/NAPI, interrupt mitigation and why one packet need not cause one interrupt. Separate kernel polling from userspace busy polling.
- Collect before/after interface/driver counters, identified IRQ rows, softirq counts and application sequence accounting during a bounded run. Correlation is not a timestamp trace of one individual packet.
- Provide an optional discovered tracepoint observation only when the running kernel exposes it; source/counter exercise stands alone. Explain aggregation, unrelated traffic, drops and CPU/scheduler effects.

**Evidence:** complete RX ownership/path diagram and a repeatable measurement record that states what each counter can and cannot establish.

## Chapter 13 — Move packet processing toward userspace (6 lessons)

### 13.1 Receive through a shared packet ring

**Result:** replace per-packet receives in the raw reader with a small TPACKET_V3 RX ring.

- Explain block/frame layout, kernel/user ownership, retirement timeout, batch traversal, acquire/release access and returning blocks promptly.
- Use one bounded ring configuration with checked offsets/lengths, poll deadline, drop statistics and complete cleanup. Do not conflate this ring with hardware descriptors.
- Reuse the same decoder and packets, and show how syscall counts and batching change. No zero-copy or throughput improvement is assumed.

**Evidence:** parser/ring bounds checks, finite namespace run and matched decoded payloads relative to the previous receiver.

### 13.2 Run a selective generic XDP program

**Result:** load a small BPF program that counts and selectively drops only a marked lab UDP flow.

- Introduce BPF bytecode, verifier, bounds checks, maps, attachment mode and XDP_PASS/DROP; labelled placement diagram distinguishes generic and native hooks.
- Start with pass/count, then drop the exact lab flow to demonstrate that its ordinary UDP receiver stops seeing those marked messages. Pass ARP, management traffic and all other traffic.
- Unsupported VLAN/IPv6/fragments or malformed/truncated headers follow the documented pass policy; never experiment with an unconditional interface-wide drop program.
- Fail if an existing XDP attachment would be replaced. Track the owned program identity and detach only that program on cleanup/error. Namespace first; physical repeat only with verified management path.

**Evidence:** verifier success plus pass/drop counters, finite sender/receiver results and proven attachment removal in the software lab.

### 13.3 Verify AF_XDP availability and understand its memory model

**Result:** confirm the existing course kernel and understand what userspace will own before creating packet rings.

- Reuse the lesson 5.2 release/config/provenance checks and require `6.12.96-hfs1-v8` with AF_XDP enabled. There is no second kernel build or mid-course switch.
- Create and close a minimal AF_XDP socket; distinguish unavailable family, privilege failure, and successful creation from a bound packet-processing socket.
- Draw UMEM chunks and the FILL, RX, TX and COMPLETION rings, distinguishing application addresses, UMEM offsets, software descriptors and the physical NIC's DMA descriptors.
- Explain copy versus zero-copy and generic versus native mode as separate choices. The pinned GENET route uses generic/copy; an enabled socket family does not add missing driver hooks.
- Establish permissions, compatible libxdp/libbpf tools and the private namespace topology for the next exercise. Readers who have booted a different kernel return to the single setup check rather than silently following a different course configuration.

**Evidence:** correct running release and feature check, minimal socket lifecycle result, and a complete ownership diagram. No Pi boot or physical networking claim follows from CI alone.

### 13.4 Redirect a lab flow into AF_XDP receive buffers

**Result:** a small userspace receiver obtains selected frames through an explicit generic-XDP/copy-mode socket.

- Explain UMEM and FILL/RX ownership using maintained libxdp helpers; allocate a bounded pool and bind the socket to the correct interface and RX queue.
- Discover queue information and count actual RX queue indexes; provide matching sockets/map entries for queues reached by the chosen flow rather than assuming queue 0. Namespace route has an explicitly configured simple queue topology.
- Redirect only fully checked marked packets with a ready matching socket; otherwise pass. Explain that a wrong queue or empty/broken redirect target can lose packets.
- Print selected mode and query actual socket options; no silent native or zero-copy fallback claims. Reuse the decoder and record sequences; prove redirection by comparing an ordinary receiver for that same flow.
- Exercise bounded pool starvation/recovery without reusing a buffer still owned by the kernel; report drops rather than manufacturing successful delivery.

**Evidence:** BPF load and namespace redirect checks, sequence accounting, ownership invariant tests and explicit copy-mode record. Physical repeat separately qualified.

### 13.5 Transmit replies and reclaim buffers correctly

**Result:** a complete AF_XDP UDP responder with all four rings and correct frame reuse.

- Add TX/COMPLETION; RX release is not TX completion. Follow ownership from receive to reply submission to completion to reuse, with a single application producer/consumer design.
- Limit replies to the lab's valid unfragmented short UDP requests. Swap addresses/ports, preserve lengths and calculate IPv4/UDP checksums explicitly; normal stack handles ARP because those frames are passed.
- Cover TX wakeups, partial ring availability, timeouts, invalid descriptors, finite drain and cleanup order. No frame address is reused twice or freed while in flight.
- Compare the same sender's replies with normal UDP. Ensure the marked flow reaches only one responder; no competing socket echo or duplicate replies.

**Evidence:** independent byte/checksum fixtures, finite request/reply namespace test, forced small-pool/error tests and cleanup/reattach checks. This is a constrained responder, not a replacement TCP/IP stack.

### 13.6 Compare paths and explain full kernel bypass

**Result:** explain the tradeoffs with an honest path matrix and reproducible experiments.

- Side-by-side diagrams: UDP socket; AF_PACKET/PACKET_MMAP; generic XDP plus AF_XDP copy; native-XDP/zero-copy on suitable hardware (conceptual); userspace NIC PMD.
- Compare semantically matched normal UDP and AF_XDP request/reply runs. Treat passive packet readers as a separate capture/batching comparison, not equivalent echo services.
- Record message size/count/rate, wall time, CPU use, loss and RTT distribution on one clock, repeated bounded runs, kernel/driver/mode and offload settings. Report sender/bridge/virtualization bottlenecks. No line-rate or universal speedup claim, and no software-link measurement represented as PHY/DMA throughput.
- Explain DPDK's PMDs, polling cost, buffer pools and device ownership. Some PMDs use kernel mechanisms or bifurcated drivers; DPDK is not a guarantee of direct hardware bypass. No instruction to unbind the Pi's network device or purchase an unsupported NIC.
- Finish with a decision guide and cleanup to the normal course-kernel network state. Outline how a later Pico/W5500 extension would move register/protocol responsibilities again; no added Pico code or promised future page in this scope.

**Evidence:** filled capability/path matrix, a runnable measurement method with sample output labelled illustrative, and complete cleanup/recovery route.

## Shared implementation and verification after approval

- Root first validates the private namespace topology and compatibility matrix on Linux, then implements shared decoder/message protocol, normal endpoint, packet reader and BPF/AF_XDP programs. C++20 for userspace; small kernel-facing BPF C; portable Python for Mac traffic where helpful. Keep examples in a networking subdirectory so existing Linux glob builds are not broken.
- Pin Linux headers/source, libxdp/libbpf, clang/LLVM BPF toolchain and packages actually used; record platform-specific build routes. The existing embedded Arm compiler is not a Linux or BPF compiler.
- Every new lesson gets its own fresh writer and separate substantive reviewer under AUTHORING_GUIDE.md. Review source claims, hardware labels, actual code, namespace/interface ownership, loader teardown, exact command context and realistic observations. Include whole checked files in unindented fences. Retain current formatting and descriptive variable names.
- Verification layers: deterministic message/frame/checksum tests with sanitizers; native Linux userspace and BPF builds; privileged BPF verifier/namespace integration in an environment whose kernel capabilities are recorded; failure/cleanup cases; single AF_XDP-enabled Pi kernel configuration/build artifacts; complete mdBook include/link/SVG checks; representative browser checks; anonymous public download/chapter checks.
- Capability-limited hosted runners explicitly report skipped privileged tests; they do not count as runtime passes. Provide an enabled Linux integration environment and acceptance checklist; do not use a skip to claim AF_XDP validated. No actual Pi or physical-link result is claimed without measurement.
- Add catalog/roadmap/glossary/download/verification entries only after user approves authoring. Preserve Chapters 0–10 and their experiments; reuse the single kernel established in 5.2 and retain its emergency recovery instructions. Publish each accepted new page using the existing public Vercel workflow.

## Primary research references

- [Pi 4 hardware specifications](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/specifications/) and [existing labelled board chapter](src/0.3-a-labelled-tour-of-your-raspberry-pi-4.md).
- [Pinned Pi 4 Device Tree](https://raw.githubusercontent.com/raspberrypi/linux/ae161617d7a4552fa91d03626b8d9f3696d17481/arch/arm/boot/dts/broadcom/bcm2711-rpi-4-b.dts), [GENET](https://raw.githubusercontent.com/raspberrypi/linux/ae161617d7a4552fa91d03626b8d9f3696d17481/drivers/net/ethernet/broadcom/genet/bcmgenet.c), [GENET MDIO/PHY](https://raw.githubusercontent.com/raspberrypi/linux/ae161617d7a4552fa91d03626b8d9f3696d17481/drivers/net/ethernet/broadcom/genet/bcmmii.c).
- [Baseline configuration](https://raw.githubusercontent.com/raspberrypi/linux/ae161617d7a4552fa91d03626b8d9f3696d17481/arch/arm64/configs/bcm2711_defconfig), [AF_XDP Kconfig](https://raw.githubusercontent.com/raspberrypi/linux/ae161617d7a4552fa91d03626b8d9f3696d17481/net/xdp/Kconfig), [Pi kernel build/install documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html).
- [Linux 6.12 DMA API](https://docs.kernel.org/6.12/core-api/dma-api-howto.html), [NAPI](https://docs.kernel.org/6.12/networking/napi.html), [packet sockets](https://man7.org/linux/man-pages/man7/packet.7.html), [packet rings](https://docs.kernel.org/6.12/networking/packet_mmap.html), [AF_XDP](https://docs.kernel.org/6.12/networking/af_xdp.html).
- [libbpf lifecycle](https://docs.kernel.org/bpf/libbpf/libbpf_overview.html), [libxdp upstream](https://github.com/xdp-project/xdp-tools/blob/main/lib/libxdp/README.org), [AF_XDP ring helper source](https://github.com/xdp-project/xdp-tools/blob/main/headers/xdp/xsk.h).
- [DPDK Ethernet PMDs](https://doc.dpdk.org/guides/prog_guide/ethdev/ethdev.html), [DPDK AF_XDP PMD](https://doc.dpdk.org/guides/nics/af_xdp.html).

Research completed 2026-11-12; revised for the user-authorised single-kernel baseline. Runtime support on the user's booted Pi is not established by research. Implementation version pins and measured results will be added only after those checks are performed.
