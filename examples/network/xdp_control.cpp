#include "xsk_common.hpp"
#include <thread>

void test_classifier(hfs::net::XdpProgram& program) {
    using namespace hfs::net;
    std::vector<std::uint8_t> valid(14 + 20 + 16, 0);
    valid[12] = 8;
    valid[14] = 0x45;
    valid[17] = 36;
    valid[23] = 17;
    valid[36] = static_cast<std::uint8_t>(lab_port >> 8);
    valid[37] = static_cast<std::uint8_t>(lab_port);
    valid[39] = 16;
    valid[42] = 'H';
    valid[43] = 'F';
    valid[44] = 'S';
    valid[45] = 1;
    auto expect = [&](const std::vector<std::uint8_t>& bytes, std::uint32_t action) {
        bpf_test_run_opts options{};
        options.sz = sizeof(options);
        options.data_in = bytes.data();
        options.data_size_in = static_cast<std::uint32_t>(bytes.size());
        options.repeat = 1;
        require(bpf_prog_test_run_opts(program.fd(), &options) == 0,
                "Kernel BPF classifier test-run failed");
        require(options.retval == action, "BPF classifier action mismatch");
    };
    program.mode(0);
    expect(valid, XDP_PASS);
    program.mode(1);
    expect(valid, XDP_DROP);
    for (auto [offset, value] :
         std::vector<std::pair<unsigned, std::uint8_t>>{{12, 0x81},
                                                        {13, 0x86},
                                                        {14, 0x46},
                                                        {14, 0x44},
                                                        {14, 0x65},
                                                        {20, 0x20},
                                                        {21, 1},
                                                        {23, 6},
                                                        {17, 35},
                                                        {37, 0},
                                                        {39, 15},
                                                        {42, 'X'},
                                                        {45, 2}}) {
        auto unsupported = valid;
        unsupported[offset] = value;
        expect(unsupported, XDP_PASS);
    }
    for (std::size_t length = 0; length < valid.size(); ++length) {
        auto truncated = valid;
        truncated.resize(length);
        // Ethernet test-run itself requires a complete minimum Ethernet header.
        if (length >= 14) {
            expect(truncated, XDP_PASS);
        }
    }
    program.mode(2);
    expect(valid, XDP_PASS); // No ready XSK entry exists in this object's map.
    std::cout
        << "classifier fixtures passed: selective pass/drop and missing-XSK fallback\n";
}

int main(int argc, char** argv) {
    try {
        using namespace hfs::net;
        require(argc >= 4 && argc <= 5,
                "Usage: xdp_control INTERFACE pass|drop|redirect|self-test SECONDS "
                "[BPF_OBJECT]");
        const std::string action = argv[2];
        require(action == "pass" || action == "drop" || action == "redirect" ||
                    action == "self-test",
                "Unknown action");
        unsigned seconds = positive_number(argv[3], 300);
        install_signal_handlers();
        unlock_memory_limit();
        XdpProgram program(argv[1],
                           argc == 5 ? argv[4] : ".build/network/lab_xdp.bpf.o");
        if (action == "self-test") {
            test_classifier(program);
            return 0;
        }
        program.mode(action == "pass" ? 0 : action == "drop" ? 1 : 2);
        program.attach();
        std::cout << "READY mode=generic action=" << action << std::endl;
        const auto deadline = SteadyClock::now() + std::chrono::seconds(seconds);
        while (!stop_requested && SteadyClock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        require(program.detach(), "Owned XDP cleanup failed");
        program.print_counters();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
