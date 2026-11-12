# Networking example contracts

The common course Pi kernel is `6.12.96-hfs1-v8`, already selected in lesson 5.2.
A Linux CI kernel may exercise the namespace software path; that is not physical
Pi/GENET/DMA validation. macOS runs only the portable fixtures and UDP sender.

## Build

**Pi/Linux — repository root:**

```sh
sudo apt update
sudo apt install build-essential clang llvm pkg-config libxdp-dev libbpf-dev libelf-dev zlib1g-dev python3 iproute2 ethtool tcpdump iputils-ping
python3 scripts/build_network.py
```

Use distribution-matched libxdp and libbpf headers/libraries together. Bookworm's
libxdp 1.3.1 and libbpf 1.1.2 APIs are the source baseline. The build report records
actual compiler/library versions and source hashes rather than pretending another
Linux distribution supplies the same package versions. `clang` must include the
BPF backend. BPF does not use the Pico compiler. Full build compiles six Linux
programs, the BPF object, and normal/sanitized wire fixtures. The BPF verifier runs
when the programs load the object, not during the clang compilation.

**Mac — repository root**, portable message/decoder/checksum tests only:

```sh
python3 scripts/build_network.py --portable-only
```

## Private topology

**Pi/Linux — repository root:**

```sh
sudo python3 scripts/network_lab.py setup
sudo python3 scripts/network_lab.py show
```

The helper owns namespace `hfs-lab`, host interface `hfs-host` at `198.18.0.1/30`,
and namespace interface `hfs-peer` at `198.18.0.2/30`. It checks collisions and
records ownership before cleanup. It creates one receive queue. It explicitly
turns off TX checksum offload on these two disposable veth interfaces so that
strict AF_XDP reply validation receives completed input checksums. It does not
change offload settings on physical interfaces or alter the default route.

## Ordinary UDP and capture

**Pi/Linux — terminal 1, repository root:**

```sh
sudo ip netns exec hfs-lab .build/network/udp_endpoint 198.18.0.2 40420 10 30
```

**Pi/Linux — terminal 2, repository root:**

```sh
python3 examples/network/udp_sender.py 198.18.0.2 --count 10
```

For each request the eight payload bytes are `H F S 01` followed by a big-endian
32-bit sequence number. UDP destination port is **40420**. The sender validates
exact replies and records RTT on its own monotonic clock. No synchronized clocks
or throughput claim is implied. A fresh sender begins at sequence 1.

Run `packet_rx hfs-peer 10 30` or `packet_ring hfs-peer 10 30` inside the namespace
in a third terminal before sending. These passive readers count marked requests;
they do not reply. Keep the ordinary endpoint running for the sender's replies.
The packet ring is a kernel/userspace packet-socket ring, not a hardware DMA ring.

## Selective generic XDP

**Pi/Linux — terminal 3, repository root**, with the normal UDP endpoint waiting:

```sh
sudo ip netns exec hfs-lab .build/network/xdp_control hfs-peer pass 30
```

The first two positional values choose the interface and action; the third is a
maximum lifetime in seconds. Actions are `pass`, `drop`, and `redirect`.
`redirect` here has an empty XSK map, so requests explicitly fall back to PASS.
With `drop`, a fresh sender with `--expect-timeout` records silence while ARP,
ICMP and unrelated traffic continue. After the controller exits or receives
Ctrl-C, it verifies the current program's identity before detaching its own
attachment. It refuses an existing native, generic or hardware XDP program and
uses UPDATE_IF_NOEXIST to prevent replacement during attachment races. It cannot
clean up after SIGKILL or power loss; inspect the actual interface/program ID
before any manual recovery. Do not detach an unidentified program.

`xdp_control hfs-peer self-test 1` loads/verifies the BPF object and runs finite
kernel classifier fixtures without attaching it. Fixtures cover marked pass/drop,
truncation, unsupported layouts/protocols/fragments/options, wrong magic/version,
and missing-XSK fallback. It requires Linux BPF privileges; unsupported capability
is a failed test, not a success or a silently skipped runtime result.

## AF_XDP receive and replies

The complete receivers require exactly one discovered RX queue named `rx-0`.
They fail on another topology instead of assuming queue 0 covers all ingress.
The namespace meets that requirement; a physical interface must pass the same
inspection. No ethtool queue reconfiguration is done by these programs.

**Pi/Linux — terminal 1, repository root:**

```sh
sudo ip netns exec hfs-lab .build/network/xsk_receiver hfs-peer 10 30
```

With that receiver active, a sender using `--expect-timeout` produces requests
that are redirected to userspace rather than the ordinary UDP endpoint. The
receiver returns after ten marked requests. It prints generic mode, queue 0 and
copy mode, verifies XDP_OPTIONS has no zero-copy flag and reports socket counters.

For replies, stop the ordinary endpoint and use:

```sh
sudo ip netns exec hfs-lab .build/network/xsk_responder hfs-peer 10 30
```

Then run a normal sender without `--expect-timeout`. The responder accepts only
this lab's bounded unfragmented, no-options IPv4 UDP request. It validates input
IPv4 and nonzero UDP checksums, swaps MAC/IP/UDP endpoints and calculates complete
reply checksums. ARP is passed to Linux; this is not a replacement network stack.

Both XSK programs accept optional `STARVE_MS FRAMES BPF_OBJECT` arguments after
the timeout. Default values are `0`, `64`, `.build/network/lab_xdp.bpf.o`.
`FRAMES` must be a power of two from 8 through 1024; the test uses 8. `STARVE_MS`
(up to 10000) delays returning consumed chunks to FILL, while still observing RX
and reclaiming TX completions. A finite burst larger than that pool causes
starvation; send another burst after the delay to observe recovery and record
actual drops. No frame is resubmitted while TX owns it. FILL, RX, TX and COMPLETION
all use libxdp's maintained acquire/release ring helpers. The application checks
each chunk's ownership and waits for TX completion before recycling it.

A timeout or signal stops the run; pending TX is not called successful. Socket
teardown stops access before UMEM unregistration/unmapping. Finite normal cleanup
sets the classifier to pass, detaches the owned program, drains completions by
the deadline and reports remaining work. The destructor order also detaches XDP
before socket/UMEM destruction when a checked operation throws.

## Automated integration and cleanup

**Pi/Linux — repository root**, with no pre-existing helper-owned lab:

```sh
sudo python3 scripts/test_network.py
```

This creates the private lab, tests normal UDP and malformed rejection, both
capture paths, BPF fixtures, pass/drop/fallback and attachment collision refusal,
AF_XDP receive versus ordinary sockets, validated replies, small pools,
starvation/recovery, timeout cleanup and subsequent attachment. It fails when
required capabilities are missing. The helper is cleaned in a finally block;
test child processes are bounded and explicitly terminated if another check fails.
Results are written to `.build/network/runtime-report.json` only after all cases
succeed. The build record is `.build/network/build-report.json`.

For manual sessions, close the programs first, verify the owned attachment is
gone and then run:

```sh
sudo python3 scripts/network_lab.py cleanup
```

Generic XDP with AF_XDP copy mode bypasses the usual protocol stack for selected
redirected frames; it still uses kernel code and copies packet bytes into UMEM.
A veth test cannot demonstrate hardware zero-copy, GENET DMA behaviour, physical
link throughput or a booted Pi result.

## Primary API references

- [Linux 6.12 AF_XDP](https://docs.kernel.org/6.12/networking/af_xdp.html).
- [libxdp 1.3.1 ring and socket API](https://github.com/xdp-project/xdp-tools/blob/v1.3.1/headers/xdp/xsk.h).
- [libbpf 1.1.2 XDP attach/detach implementation](https://github.com/libbpf/libbpf/blob/v1.1.2/src/netlink.c).
- [Linux 6.12 AF_XDP kernel implementation](https://github.com/torvalds/linux/blob/v6.12/net/xdp/xsk.c).
