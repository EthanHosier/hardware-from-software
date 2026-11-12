#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <string>
#include <sstream>
#include <iomanip>

namespace hfs::net {
constexpr std::uint16_t lab_port = 40420;
constexpr std::size_t message_size = 8;
constexpr std::size_t ethernet_size = 14;

inline std::uint16_t read16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>((std::uint16_t{bytes[offset]} << 8) |
                                      bytes[offset + 1]);
}
inline std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return (std::uint32_t{read16(bytes, offset)} << 16) | read16(bytes, offset + 2);
}
inline void write16(std::span<std::uint8_t> bytes, std::size_t offset,
                    std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8);
    bytes[offset + 1] = static_cast<std::uint8_t>(value);
}
inline std::array<std::uint8_t, message_size> encode_message(std::uint32_t sequence) {
    return {'H',
            'F',
            'S',
            1,
            static_cast<std::uint8_t>(sequence >> 24),
            static_cast<std::uint8_t>(sequence >> 16),
            static_cast<std::uint8_t>(sequence >> 8),
            static_cast<std::uint8_t>(sequence)};
}
inline std::optional<std::uint32_t>
decode_message(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != message_size || bytes[0] != 'H' || bytes[1] != 'F' ||
        bytes[2] != 'S' || bytes[3] != 1) {
        return std::nullopt;
    }
    return read32(bytes, 4);
}

enum class Kind { arp, udp, unsupported, malformed };
struct DecodedFrame {
    Kind kind = Kind::malformed;
    const char* reason = "truncated Ethernet header";
    std::size_t ipv4_header_size = 0;
    std::size_t ip_total_size = 0;
    std::size_t udp_offset = 0;
    std::size_t payload_offset = 0;
    std::size_t payload_size = 0;
    std::uint16_t source_port = 0;
    std::uint16_t destination_port = 0;
    std::optional<std::uint32_t> sequence;
};

inline DecodedFrame decode_frame(std::span<const std::uint8_t> bytes) {
    DecodedFrame decoded;
    if (bytes.size() < ethernet_size) {
        return decoded;
    }
    auto fail = [&](Kind kind, const char* reason) {
        decoded.kind = kind;
        decoded.reason = reason;
        return decoded;
    };
    const auto ether_type = read16(bytes, 12);
    if (ether_type == 0x0806) {
        if (bytes.size() < 42) {
            return fail(Kind::malformed, "truncated Ethernet/IPv4 ARP");
        }
        if (read16(bytes, 14) != 1 || read16(bytes, 16) != 0x0800 || bytes[18] != 6 ||
            bytes[19] != 4 || (read16(bytes, 20) != 1 && read16(bytes, 20) != 2)) {
            return fail(Kind::unsupported, "unsupported ARP layout or opcode");
        }
        return fail(Kind::arp, "Ethernet/IPv4 ARP");
    }
    if (ether_type != 0x0800) {
        return fail(Kind::unsupported, ether_type == 0x8100 || ether_type == 0x88a8
                                           ? "VLAN is outside this decoder"
                                           : "non-IPv4 EtherType");
    }
    if (bytes.size() < ethernet_size + 20) {
        return fail(Kind::malformed, "truncated IPv4 header");
    }
    if ((bytes[14] >> 4) != 4 || (bytes[14] & 15) < 5) {
        return fail(Kind::malformed, "invalid IPv4 version/IHL");
    }
    decoded.ipv4_header_size = std::size_t{bytes[14] & 15u} * 4;
    decoded.ip_total_size = read16(bytes, 16);
    if (decoded.ip_total_size < decoded.ipv4_header_size ||
        decoded.ip_total_size > bytes.size() - ethernet_size) {
        return fail(Kind::malformed, "invalid/truncated IPv4 total length");
    }
    if ((read16(bytes, 20) & 0x3fff) != 0) {
        return fail(Kind::unsupported, "IPv4 fragments are outside this decoder");
    }
    if (bytes[23] != 17) {
        return fail(Kind::unsupported, "non-UDP IPv4 protocol");
    }
    if (decoded.ip_total_size - decoded.ipv4_header_size < 8) {
        return fail(Kind::malformed, "truncated UDP header");
    }
    decoded.udp_offset = ethernet_size + decoded.ipv4_header_size;
    const auto udp_size = read16(bytes, decoded.udp_offset + 4);
    if (udp_size < 8 || udp_size != decoded.ip_total_size - decoded.ipv4_header_size) {
        return fail(Kind::malformed, "UDP and IPv4 lengths disagree");
    }
    decoded.source_port = read16(bytes, decoded.udp_offset);
    decoded.destination_port = read16(bytes, decoded.udp_offset + 2);
    decoded.payload_offset = decoded.udp_offset + 8;
    decoded.payload_size = udp_size - 8;
    decoded.sequence =
        decode_message(bytes.subspan(decoded.payload_offset, decoded.payload_size));
    return fail(Kind::udp, "unfragmented IPv4 UDP");
}

// Accumulate network-order words, including an odd final byte. These helpers
// operate on bytes; callers never dereference potentially unaligned IP structs.
inline std::uint32_t checksum_sum(std::span<const std::uint8_t> bytes,
                                  std::uint32_t sum = 0) {
    std::size_t offset = 0;
    for (; offset + 1 < bytes.size(); offset += 2) {
        sum += read16(bytes, offset);
    }
    if (offset < bytes.size()) {
        sum += std::uint32_t{bytes[offset]} << 8;
    }
    return sum;
}
inline std::uint16_t checksum_finish(std::uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return static_cast<std::uint16_t>(~sum);
}
inline std::uint16_t checksum(std::span<const std::uint8_t> bytes) {
    return checksum_finish(checksum_sum(bytes));
}
inline bool make_udp_reply(std::span<std::uint8_t> bytes) {
    const auto decoded = decode_frame(bytes);
    if (decoded.kind != Kind::udp || decoded.destination_port != lab_port ||
        decoded.ipv4_header_size != 20 || !decoded.sequence) {
        return false;
    }
    if (checksum(bytes.subspan(14, decoded.ipv4_header_size)) != 0) {
        return false;
    }
    const auto incoming_udp_size = decoded.payload_size + 8;
    if (read16(bytes, decoded.udp_offset + 6) != 0) {
        auto incoming_sum = checksum_sum(bytes.subspan(26, 8));
        incoming_sum += 17 + static_cast<std::uint32_t>(incoming_udp_size);
        if (checksum_finish(checksum_sum(
                bytes.subspan(decoded.udp_offset, incoming_udp_size), incoming_sum)) !=
            0) {
            return false;
        }
    }
    for (std::size_t index = 0; index < 6; ++index) {
        std::swap(bytes[index], bytes[index + 6]);
    }
    for (std::size_t index = 0; index < 4; ++index) {
        std::swap(bytes[26 + index], bytes[30 + index]);
    }
    write16(bytes, decoded.udp_offset, decoded.destination_port);
    write16(bytes, decoded.udp_offset + 2, decoded.source_port);
    bytes[22] = 64; // A new local reply gets a fresh IPv4 TTL.
    write16(bytes, 24, 0);
    write16(bytes, 24, checksum(bytes.subspan(14, decoded.ipv4_header_size)));
    write16(bytes, decoded.udp_offset + 6, 0);
    const auto udp_size = decoded.payload_size + 8;
    auto sum = checksum_sum(bytes.subspan(26, 8));
    sum += 17 + static_cast<std::uint32_t>(udp_size);
    auto udp_checksum =
        checksum_finish(checksum_sum(bytes.subspan(decoded.udp_offset, udp_size), sum));
    write16(bytes, decoded.udp_offset + 6, udp_checksum == 0 ? 0xffff : udp_checksum);
    return true;
}
inline std::string ipv4_text(std::span<const std::uint8_t> bytes) {
    return std::to_string(bytes[0]) + "." + std::to_string(bytes[1]) + "." +
           std::to_string(bytes[2]) + "." + std::to_string(bytes[3]);
}
inline std::string mac_text(std::span<const std::uint8_t> bytes) {
    std::ostringstream text;
    for (std::size_t index = 0; index < 6; ++index) {
        if (index != 0) {
            text << ':';
        }
        text << std::hex << std::setw(2) << std::setfill('0') << unsigned{bytes[index]};
    }
    return text.str();
}
inline std::string describe_frame(std::span<const std::uint8_t> bytes) {
    const auto decoded = decode_frame(bytes);
    std::ostringstream record;
    record << "{\"event\":\"frame\",\"captured_bytes\":" << bytes.size()
           << ",\"reason\":\"" << decoded.reason << "\"";
    if (bytes.size() >= ethernet_size) {
        record << ",\"destination_mac\":\"" << mac_text(bytes.first(6))
               << "\",\"source_mac\":\"" << mac_text(bytes.subspan(6, 6))
               << "\",\"ether_type\":" << read16(bytes, 12);
    }
    if (decoded.kind == Kind::arp) {
        record << ",\"arp_opcode\":" << read16(bytes, 20) << ",\"sender_ipv4\":\""
               << ipv4_text(bytes.subspan(28, 4)) << "\",\"target_ipv4\":\""
               << ipv4_text(bytes.subspan(38, 4)) << "\"";
    } else if (decoded.kind == Kind::udp) {
        record << ",\"source_ipv4\":\"" << ipv4_text(bytes.subspan(26, 4))
               << "\",\"destination_ipv4\":\"" << ipv4_text(bytes.subspan(30, 4))
               << "\",\"ipv4_header_bytes\":" << decoded.ipv4_header_size
               << ",\"ipv4_total_bytes\":" << decoded.ip_total_size
               << ",\"source_port\":" << decoded.source_port
               << ",\"destination_port\":" << decoded.destination_port
               << ",\"payload_bytes\":" << decoded.payload_size;
        if (decoded.sequence) {
            record << ",\"sequence\":" << *decoded.sequence;
        }
    }
    record << '}';
    return record.str();
}

} // namespace hfs::net
