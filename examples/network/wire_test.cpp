#include "wire.hpp"
#include <cassert>
#include <iostream>
#include <vector>

namespace {
// Independent byte-at-a-time reference. Do not validate a checksum helper by
// calling that same helper to construct the expected packet.
std::uint16_t reference_checksum(std::span<const std::uint8_t> bytes) {
    std::uint64_t sum = 0;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        sum += static_cast<std::uint64_t>(bytes[index]) << (index % 2 == 0 ? 8 : 0);
    }
    while (sum > 65535) {
        sum = (sum & 65535) + (sum >> 16);
    }
    return static_cast<std::uint16_t>(65535 - sum);
}
void put16(std::span<std::uint8_t> bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value / 256);
    bytes[offset + 1] = static_cast<std::uint8_t>(value % 256);
}
std::uint16_t udp_reference(std::span<const std::uint8_t> bytes,
                            std::size_t udp_offset = 34) {
    const auto udp_size = bytes.size() - udp_offset;
    std::vector<std::uint8_t> pseudo(bytes.begin() + 26, bytes.begin() + 34);
    pseudo.insert(pseudo.end(), {0, 17, static_cast<std::uint8_t>(udp_size >> 8),
                                 static_cast<std::uint8_t>(udp_size)});
    pseudo.insert(pseudo.end(), bytes.begin() + udp_offset, bytes.end());
    return reference_checksum(pseudo);
}
std::vector<std::uint8_t> request() {
    std::vector<std::uint8_t> bytes{
        2,    0,    0,    0,    0, 2,  2,  0, 0,   0,   0,   1, 8, 0,   0x45, 0, 0,
        36,   0x12, 0x34, 0x40, 0, 64, 17, 0, 0,   198, 18,  0, 1, 198, 18,   0, 2,
        0xc3, 0x50, 0x9d, 0xe4, 0, 16, 0,  0, 'H', 'F', 'S', 1, 1, 2,   3,    4};
    static_assert(hfs::net::lab_port == 40420);
    put16(bytes, 24, reference_checksum(std::span(bytes).subspan(14, 20)));
    const auto udp_checksum = udp_reference(bytes);
    put16(bytes, 40, udp_checksum == 0 ? 0xffff : udp_checksum);
    return bytes;
}
} // namespace

int main() {
    using namespace hfs::net;
    assert((encode_message(0x01020304) ==
            std::array<std::uint8_t, 8>{'H', 'F', 'S', 1, 1, 2, 3, 4}));
    assert(decode_message(encode_message(0xffffffff)) == 0xffffffff);
    assert(decode_message(encode_message(0)) == 0);
    assert(!decode_message(std::array<std::uint8_t, 7>{}));
    auto message = encode_message(1);
    for (std::size_t index = 0; index < 4; ++index) {
        auto changed = message;
        changed[index] ^= 0xff;
        assert(!decode_message(changed));
    }
    assert(checksum(std::array<std::uint8_t, 8>{0, 1, 0xf2, 3, 0xf4, 0xf5, 0xf6,
                                                0xf7}) == 0x220d);
    assert(checksum(std::array<std::uint8_t, 3>{1, 2, 3}) == 0xfbfd);
    auto frame = request();
    const auto decoded = decode_frame(frame);
    assert(decoded.kind == Kind::udp && decoded.sequence == 0x01020304);
    assert(decoded.destination_port == lab_port && decoded.source_port == 50000);
    assert(decoded.ipv4_header_size == 20 && decoded.ip_total_size == 36);
    assert(decoded.payload_offset == 42 && decoded.payload_size == 8);
    assert(describe_frame(frame).find("198.18.0.1") != std::string::npos);
    for (std::size_t length = 0; length < frame.size(); ++length) {
        assert(decode_frame(std::span(frame).first(length)).kind == Kind::malformed);
    }
    auto padded = frame;
    padded.resize(60, 0xff);
    assert(decode_frame(padded).sequence == 0x01020304);
    std::vector<std::uint8_t> unaligned(1, 0);
    unaligned.insert(unaligned.end(), frame.begin(), frame.end());
    assert(decode_frame(std::span(unaligned).subspan(1)).kind == Kind::udp);

    for (auto ether_type : {0x8100, 0x88a8, 0x86dd}) {
        auto changed = frame;
        put16(changed, 12, static_cast<std::uint16_t>(ether_type));
        assert(decode_frame(changed).kind == Kind::unsupported);
        assert(!make_udp_reply(changed));
    }
    for (auto fragment : {0x2000, 1}) {
        auto changed = frame;
        put16(changed, 20, static_cast<std::uint16_t>(fragment));
        assert(decode_frame(changed).kind == Kind::unsupported);
        assert(!make_udp_reply(changed));
    }
    for (auto version_ihl : {0x44, 0x65, 0x4f}) {
        auto changed = frame;
        changed[14] = static_cast<std::uint8_t>(version_ihl);
        assert(decode_frame(changed).kind == Kind::malformed);
    }
    for (auto ip_size : {0, 19, 35, 37, 65535}) {
        auto changed = frame;
        put16(changed, 16, static_cast<std::uint16_t>(ip_size));
        assert(decode_frame(changed).kind == Kind::malformed);
    }
    for (auto udp_size : {0, 7, 15, 17, 65535}) {
        auto changed = frame;
        put16(changed, 38, static_cast<std::uint16_t>(udp_size));
        assert(decode_frame(changed).kind == Kind::malformed);
    }
    auto options = frame;
    options.insert(options.begin() + 34, 4, 0);
    options[14] = 0x46;
    put16(options, 16, 40);
    put16(options, 24, 0);
    put16(options, 24, reference_checksum(std::span(options).subspan(14, 24)));
    assert(decode_frame(options).sequence == 0x01020304);
    assert(
        !make_udp_reply(options)); // Options require semantics beyond this responder.

    auto bad_ip_checksum = frame;
    bad_ip_checksum[24] ^= 1;
    assert(!make_udp_reply(bad_ip_checksum));
    auto bad_udp_checksum = frame;
    bad_udp_checksum.back() ^= 1;
    assert(!make_udp_reply(bad_udp_checksum));
    auto bad_magic = frame;
    bad_magic[42] = 'X';
    assert(!make_udp_reply(bad_magic));
    auto wrong_port = frame;
    put16(wrong_port, 36, 1);
    assert(!make_udp_reply(wrong_port));
    auto reply = frame;
    assert(make_udp_reply(reply));
    assert(reply.size() == frame.size());
    assert(reference_checksum(std::span(reply).subspan(14, 20)) == 0);
    assert(udp_reference(reply) == 0);
    assert(std::equal(reply.begin(), reply.begin() + 6, frame.begin() + 6));
    assert(std::equal(reply.begin() + 26, reply.begin() + 30, frame.begin() + 30));
    assert(decode_frame(reply).source_port == lab_port);
    assert(decode_frame(reply).destination_port == 50000);
    assert(decode_frame(reply).sequence == decoded.sequence);
    auto no_udp_checksum = frame;
    put16(no_udp_checksum, 40, 0); // Optional only for IPv4 UDP input.
    assert(make_udp_reply(no_udp_checksum));
    assert(udp_reference(no_udp_checksum) == 0 && read16(no_udp_checksum, 40) != 0);

    std::array<std::uint8_t, 42> arp{};
    put16(arp, 12, 0x0806);
    put16(arp, 14, 1);
    put16(arp, 16, 0x0800);
    arp[18] = 6;
    arp[19] = 4;
    put16(arp, 20, 1);
    assert(decode_frame(arp).kind == Kind::arp);
    put16(arp, 20, 3);
    assert(decode_frame(arp).kind == Kind::unsupported);

    // Deterministic malformed-byte exploration under ASan/UBSan. This does not
    // establish protocol completeness; it exercises bounds at many inputs.
    std::uint32_t generator = 1;
    for (unsigned iteration = 0; iteration < 20000; ++iteration) {
        generator = generator * 1664525 + 1013904223;
        std::vector<std::uint8_t> bytes(generator % 200);
        for (auto& byte : bytes) {
            generator = generator * 1664525 + 1013904223;
            byte = static_cast<std::uint8_t>(generator >> 24);
        }
        const auto result = decode_frame(bytes);
        if (result.kind == Kind::udp) {
            assert(result.payload_offset + result.payload_size <= bytes.size());
        }
        make_udp_reply(bytes);
    }
    std::cout << "wire fixtures and 20000 bounded malformed inputs passed\n";
}
