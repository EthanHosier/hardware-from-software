#pragma once

#include "wire.hpp"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <xdp/xsk.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace hfs::net {
using SteadyClock = std::chrono::steady_clock;
inline volatile std::sig_atomic_t stop_requested = 0;
inline void request_stop(int) {
    stop_requested = 1;
}
inline void install_signal_handlers() {
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);
}
inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
inline unsigned positive_number(const char* text, unsigned maximum) {
    std::string input(text);
    std::size_t consumed = 0;
    unsigned long result = std::stoul(input, &consumed);
    require(consumed == input.size() && result > 0 && result <= maximum,
            "Expected a positive number within the documented bound");
    return static_cast<unsigned>(result);
}
inline void unlock_memory_limit() {
    rlimit limit{RLIM_INFINITY, RLIM_INFINITY};
    if (setrlimit(RLIMIT_MEMLOCK, &limit) != 0) {
        // New kernels account BPF memory using memcg; a finite limit may suffice.
        std::cerr << "Note: cannot raise memlock limit: " << std::strerror(errno)
                  << '\n';
    }
}

class XdpProgram {
    bpf_object* object_ = nullptr;
    int interface_index_ = 0;
    int program_fd_ = -1;
    std::uint32_t program_id_ = 0;
    bool attached_ = false;

  public:
    XdpProgram(const std::string& interface, const char* object_path) {
        interface_index_ = static_cast<int>(if_nametoindex(interface.c_str()));
        require(interface_index_ != 0, "Unknown interface");
        for (std::uint32_t attachment_mode :
             {XDP_FLAGS_SKB_MODE, XDP_FLAGS_DRV_MODE, XDP_FLAGS_HW_MODE}) {
            std::uint32_t existing_id = 0;
            int result =
                bpf_xdp_query_id(interface_index_, attachment_mode, &existing_id);
            require(result == 0, "Cannot inspect existing XDP attachment");
            require(existing_id == 0,
                    "Interface already has an XDP program; refusing replacement");
        }
        object_ = bpf_object__open_file(object_path, nullptr);
        long open_error = libbpf_get_error(object_);
        if (open_error) {
            object_ = nullptr;
            throw std::runtime_error("Cannot open BPF object");
        }
        try {
            require(bpf_object__load(object_) == 0, "BPF load/verifier failed");
            bpf_program* program = bpf_object__find_program_by_name(object_, "lab_xdp");
            require(program != nullptr, "Missing lab_xdp program");
            program_fd_ = bpf_program__fd(program);
            bpf_prog_info information{};
            std::uint32_t information_size = sizeof(information);
            require(bpf_obj_get_info_by_fd(program_fd_, &information,
                                           &information_size) == 0,
                    "Cannot inspect loaded program identity");
            program_id_ = information.id;
        } catch (...) {
            bpf_object__close(object_);
            object_ = nullptr;
            throw;
        }
    }
    XdpProgram(const XdpProgram&) = delete;
    XdpProgram& operator=(const XdpProgram&) = delete;
    int map(const char* name) const {
        int descriptor = bpf_object__find_map_fd_by_name(object_, name);
        require(descriptor >= 0, std::string("Missing BPF map: ") + name);
        return descriptor;
    }
    int fd() const {
        return program_fd_;
    }
    void mode(std::uint32_t value) {
        std::uint32_t key = 0;
        require(bpf_map_update_elem(map("mode"), &key, &value, BPF_ANY) == 0,
                "Cannot set XDP mode");
    }
    void attach() {
        require(bpf_xdp_attach(interface_index_, program_fd_,
                               XDP_FLAGS_SKB_MODE | XDP_FLAGS_UPDATE_IF_NOEXIST,
                               nullptr) == 0,
                "Cannot attach generic XDP without replacing another program");
        attached_ = true;
    }
    bool detach() noexcept {
        if (!attached_) {
            return true;
        }
        std::uint32_t current_id = 0;
        if (bpf_xdp_query_id(interface_index_, XDP_FLAGS_SKB_MODE, &current_id) != 0) {
            std::cerr << "Cannot verify XDP owner; refusing an unverified detach\n";
            return false;
        }
        if (current_id != program_id_) {
            std::cerr << "XDP owner changed; leaving the current program untouched\n";
            attached_ = false;
            return false;
        }
        bpf_xdp_attach_opts options{};
        options.sz = sizeof(options);
        options.old_prog_fd = program_fd_;
        if (bpf_xdp_detach(interface_index_, XDP_FLAGS_SKB_MODE, &options) != 0) {
            std::cerr << "Cannot detach owned XDP program\n";
            return false;
        }
        attached_ = false;
        return true;
    }
    void print_counters() const {
        std::cout << "selected_flow";
        const char* labels[] = {"pass", "drop", "redirect"};
        for (std::uint32_t key = 0; key < 3; ++key) {
            std::uint64_t value = 0;
            require(bpf_map_lookup_elem(map("counters"), &key, &value) == 0,
                    "Cannot read XDP counters");
            std::cout << ' ' << labels[key] << '=' << value;
        }
        std::cout << '\n';
    }
    ~XdpProgram() {
        detach();
        bpf_object__close(object_);
    }
};

inline void require_single_receive_queue(const std::string& interface) {
    std::vector<std::string> queues;
    for (const auto& entry : std::filesystem::directory_iterator(
             std::filesystem::path("/sys/class/net") / interface / "queues")) {
        const std::string name = entry.path().filename();
        if (name.starts_with("rx-")) {
            queues.push_back(name);
        }
    }
    require(queues.size() == 1 && queues.front() == "rx-0",
            "This bounded lab requires exactly one discovered receive queue, rx-0");
}

class XskBuffers {
    static constexpr std::uint32_t chunk_size = 4096;
    xsk_umem* umem_ = nullptr;
    xsk_socket* socket_ = nullptr;
    void* memory_ = MAP_FAILED;
    std::size_t memory_size_ = 0;
    enum class Owner { free, fill, application, transmit };
    std::vector<Owner> owners_;
    std::vector<std::uint64_t> available_;
    std::size_t pending_transmits_ = 0;

  public:
    xsk_ring_prod fill{};
    xsk_ring_cons completion{};
    xsk_ring_cons receive{};
    xsk_ring_prod transmit{};

    XskBuffers(const std::string& interface, unsigned frame_count) {
        require_single_receive_queue(interface);
        require(frame_count >= 8 && frame_count <= 1024 &&
                    (frame_count & (frame_count - 1)) == 0,
                "Frame count must be a power of two from 8 through 1024");
        memory_size_ = std::size_t(frame_count) * chunk_size;
        memory_ = mmap(nullptr, memory_size_, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        require(memory_ != MAP_FAILED, "Cannot allocate UMEM");
        try {
            xsk_umem_config umem_config{};
            umem_config.fill_size = frame_count;
            umem_config.comp_size = frame_count;
            umem_config.frame_size = chunk_size;
            require(xsk_umem__create(&umem_, memory_, memory_size_, &fill, &completion,
                                     &umem_config) == 0,
                    "Cannot register UMEM");
            xsk_socket_config socket_config{};
            socket_config.rx_size = frame_count;
            socket_config.tx_size = frame_count;
            socket_config.libxdp_flags = XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD;
            socket_config.xdp_flags = XDP_FLAGS_SKB_MODE;
            socket_config.bind_flags = XDP_COPY | XDP_USE_NEED_WAKEUP;
            require(xsk_socket__create(&socket_, interface.c_str(), 0, umem_, &receive,
                                       &transmit, &socket_config) == 0,
                    "Cannot bind AF_XDP socket in explicit copy mode");
            xdp_options options{};
            socklen_t options_size = sizeof(options);
            require(getsockopt(fd(), SOL_XDP, XDP_OPTIONS, &options, &options_size) ==
                        0,
                    "Cannot query AF_XDP mode");
            require((options.flags & XDP_OPTIONS_ZEROCOPY) == 0,
                    "Unexpected zero-copy socket");
            owners_.resize(frame_count, Owner::free);
            for (unsigned index = 0; index < frame_count; ++index) {
                available_.push_back(std::uint64_t(index) * chunk_size);
            }
            replenish();
        } catch (...) {
            if (socket_) {
                xsk_socket__delete(socket_);
            }
            if (umem_) {
                xsk_umem__delete(umem_);
            }
            munmap(memory_, memory_size_);
            throw;
        }
    }
    XskBuffers(const XskBuffers&) = delete;
    XskBuffers& operator=(const XskBuffers&) = delete;
    int fd() const {
        return xsk_socket__fd(socket_);
    }
    std::size_t pending() const {
        return pending_transmits_;
    }
    std::span<std::uint8_t> packet(std::uint64_t address, std::uint32_t length) {
        require(address <= memory_size_ && length <= memory_size_ - address,
                "RX descriptor exceeds UMEM");
        require(length <= chunk_size - (address % chunk_size),
                "RX descriptor crosses a chunk");
        const auto frame_index = address / chunk_size;
        require(address < memory_size_ && owners_[frame_index] == Owner::fill,
                "RX descriptor does not name a kernel-owned FILL chunk");
        owners_[frame_index] = Owner::application;
        return {static_cast<std::uint8_t*>(memory_) + address, length};
    }
    void recycle(std::uint64_t address) {
        const std::uint64_t base = address - address % chunk_size;
        require(base < memory_size_ && owners_[base / chunk_size] == Owner::application,
                "Cannot recycle a frame owned by TX");
        owners_[base / chunk_size] = Owner::free;
        available_.push_back(base);
    }
    void replenish() {
        std::uint32_t producer_index = 0;
        while (!available_.empty() &&
               xsk_ring_prod__reserve(&fill, 1, &producer_index) == 1) {
            const auto address = available_.back();
            require(owners_[address / chunk_size] == Owner::free,
                    "FILL chunk submitted twice");
            owners_[address / chunk_size] = Owner::fill;
            *xsk_ring_prod__fill_addr(&fill, producer_index) = address;
            available_.pop_back();
            xsk_ring_prod__submit(&fill, 1);
        }
    }
    bool send_frame(std::uint64_t address, std::uint32_t length) {
        std::uint32_t producer_index = 0;
        const std::uint64_t base = address - address % chunk_size;
        require(base < memory_size_ && owners_[base / chunk_size] == Owner::application,
                "Frame submitted twice to TX");
        if (xsk_ring_prod__reserve(&transmit, 1, &producer_index) != 1) {
            return false;
        }
        xdp_desc* descriptor = xsk_ring_prod__tx_desc(&transmit, producer_index);
        descriptor->addr = address;
        descriptor->len = length;
        descriptor->options = 0;
        owners_[base / chunk_size] = Owner::transmit;
        ++pending_transmits_;
        xsk_ring_prod__submit(&transmit, 1);
        return true;
    }
    void kick_transmit() {
        if (pending_transmits_ && xsk_ring_prod__needs_wakeup(&transmit)) {
            int result = sendto(fd(), nullptr, 0, MSG_DONTWAIT, nullptr, 0);
            require(result >= 0 || errno == EAGAIN || errno == ENOBUFS ||
                        errno == EINTR,
                    "AF_XDP TX wakeup failed");
        }
    }
    void reclaim_completions() {
        std::uint32_t consumer_index = 0;
        unsigned count = xsk_ring_cons__peek(&completion, 64, &consumer_index);
        for (unsigned index = 0; index < count; ++index) {
            const std::uint64_t address =
                *xsk_ring_cons__comp_addr(&completion, consumer_index + index);
            const std::uint64_t base = address - address % chunk_size;
            require(base < memory_size_ &&
                        owners_[base / chunk_size] == Owner::transmit,
                    "Unexpected or duplicate TX completion");
            owners_[base / chunk_size] = Owner::application;
            --pending_transmits_;
            recycle(base);
        }
        xsk_ring_cons__release(&completion, count);
    }
    void poll_once(SteadyClock::time_point deadline) {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                             deadline - SteadyClock::now())
                             .count();
        if (remaining <= 0) {
            return;
        }
        pollfd descriptor{fd(), POLLIN, 0};
        int result = poll(&descriptor, 1,
                          static_cast<int>(std::min<std::int64_t>(remaining, 20)));
        require(result >= 0 || errno == EINTR, "AF_XDP poll failed");
        require((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) == 0,
                "AF_XDP socket poll reported an error");
    }
    void print_statistics() const {
        xdp_statistics statistics{};
        socklen_t size = sizeof(statistics);
        require(getsockopt(fd(), SOL_XDP, XDP_STATISTICS, &statistics, &size) == 0,
                "Cannot obtain AF_XDP statistics");
        std::cout << "xsk rx_dropped=" << statistics.rx_dropped
                  << " rx_invalid_descs=" << statistics.rx_invalid_descs
                  << " tx_invalid_descs=" << statistics.tx_invalid_descs << '\n';
        require(statistics.rx_invalid_descs == 0 && statistics.tx_invalid_descs == 0,
                "Invalid AF_XDP descriptors reported");
    }
    ~XskBuffers() {
        // The socket must stop using UMEM before its registration/mapping is freed.
        if (socket_) {
            xsk_socket__delete(socket_);
        }
        if (umem_) {
            xsk_umem__delete(umem_);
        }
        if (memory_ != MAP_FAILED) {
            munmap(memory_, memory_size_);
        }
    }
};

inline int run_xsk(int argc, char** argv, bool reply) {
    require(argc >= 4 && argc <= 7,
            "Usage: xsk_receiver/responder INTERFACE COUNT TIMEOUT_SECONDS [STARVE_MS "
            "[FRAMES [BPF_OBJECT]]]");
    const std::string interface = argv[1];
    unsigned target_count = positive_number(argv[2], 100000);
    unsigned timeout_seconds = positive_number(argv[3], 300);
    unsigned starvation_ms = argc >= 5 ? static_cast<unsigned>(std::stoul(argv[4])) : 0;
    require(starvation_ms <= 10000, "STARVE_MS must be at most 10000");
    unsigned frames = argc >= 6 ? positive_number(argv[5], 1024) : 64;
    const char* object_path = argc >= 7 ? argv[6] : ".build/network/lab_xdp.bpf.o";
    install_signal_handlers();
    unlock_memory_limit();
    XskBuffers buffers(interface, frames);
    // Declared last: detach the program before socket/UMEM teardown on errors too.
    XdpProgram program(interface, object_path);
    std::uint32_t queue_id = 0;
    int socket_fd = buffers.fd();
    require(bpf_map_update_elem(program.map("sockets"), &queue_id, &socket_fd,
                                BPF_ANY) == 0,
            "Cannot register the ready socket in XSKMAP");
    program.mode(2);
    program.attach();
    std::cout << "READY mode=generic copy=1 queue=0 frames=" << frames << std::endl;
    const auto start = SteadyClock::now();
    const auto deadline = start + std::chrono::seconds(timeout_seconds);
    const auto replenish_after = start + std::chrono::milliseconds(starvation_ms);
    unsigned received_count = 0;
    unsigned transmitted_count = 0;
    while (!stop_requested && SteadyClock::now() < deadline &&
           received_count < target_count) {
        buffers.reclaim_completions();
        if (SteadyClock::now() >= replenish_after) {
            buffers.replenish();
        }
        std::uint32_t consumer_index = 0;
        unsigned count = xsk_ring_cons__peek(&buffers.receive, 32, &consumer_index);
        for (unsigned index = 0; index < count; ++index) {
            const xdp_desc descriptor =
                *xsk_ring_cons__rx_desc(&buffers.receive, consumer_index + index);
            auto bytes = buffers.packet(descriptor.addr, descriptor.len);
            const auto decoded = decode_frame(bytes);
            const bool selected =
                decoded.sequence && decoded.destination_port == lab_port;
            const bool valid_reply = reply && selected && make_udp_reply(bytes);
            if (selected && (!reply || valid_reply)) {
                ++received_count;
                std::cout << "seq=" << *decoded.sequence << '\n';
            }
            if (valid_reply) {
                if (buffers.send_frame(descriptor.addr, descriptor.len)) {
                    ++transmitted_count;
                } else {
                    buffers.recycle(descriptor.addr);
                }
            } else {
                buffers.recycle(descriptor.addr);
            }
        }
        xsk_ring_cons__release(&buffers.receive, count);
        buffers.kick_transmit();
        if (count == 0) {
            buffers.poll_once(deadline);
        }
    }
    // Stop new redirects before draining TX; packets arriving now take normal path.
    program.mode(0);
    require(program.detach(), "Owned XDP cleanup failed");
    while (buffers.pending() && SteadyClock::now() < deadline && !stop_requested) {
        buffers.kick_transmit();
        buffers.reclaim_completions();
        buffers.poll_once(deadline);
    }
    buffers.print_statistics();
    program.print_counters();
    std::cout << "received=" << received_count << " transmitted=" << transmitted_count
              << " pending=" << buffers.pending() << '\n';
    require(
        buffers.pending() == 0,
        "Deadline ended before TX completion; socket teardown cancels remaining work");
    require(received_count >= target_count,
            "Receive deadline expired before COUNT packets");
    require(!reply || transmitted_count == received_count,
            "TX ring lacked space for a reply");
    return 0;
}
} // namespace hfs::net
