#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hfs {
// Fixed storage; feed one byte at a time. Overlong input is discarded to LF.
class Line {
    char buffer_[64]{};
    std::size_t buffer_size_ = 0;
    bool discarding_line_ = false;

  public:
    enum class Result { none, ready, overflow };
    const char* text() const {
        return buffer_;
    }
    Result feed(char byte) {
        if (byte == '\r') {
            return Result::none;
        }
        if (byte == '\n') {
            if (discarding_line_) {
                discarding_line_ = false;
                buffer_size_ = 0;
                return Result::overflow;
            }
            buffer_[buffer_size_] = '\0';
            buffer_size_ = 0;
            return Result::ready;
        }
        if (discarding_line_) {
            return Result::none;
        }
        if (static_cast<unsigned char>(byte) < 32 ||
            static_cast<unsigned char>(byte) > 126) {
            discarding_line_ = true;
            buffer_size_ = 0;
            return Result::none;
        }
        if (buffer_size_ == sizeof(buffer_) - 1) {
            discarding_line_ = true;
            buffer_size_ = 0;
            return Result::none;
        }
        buffer_[buffer_size_++] = byte;
        return Result::none;
    }
};
enum class Op { ping, led, count, temp };
struct Request {
    std::uint32_t id = 0;
    Op op = Op::ping;
    bool led = false;
};
inline bool parse(const char* line, Request& output_request) {
    const char* cursor = line;
    std::uint32_t id = 0;
    unsigned digit_count = 0;
    while (*cursor >= '0' && *cursor <= '9') {
        if (++digit_count > 6) {
            return false;
        }
        id = id * 10u + static_cast<unsigned>(*cursor++ - '0');
    }
    if (!id || *cursor++ != ' ') {
        return false;
    }
    Request parsed_request;
    parsed_request.id = id;
    if (std::strcmp(cursor, "PING") == 0) {
        parsed_request.op = Op::ping;
    } else if (std::strcmp(cursor, "COUNT") == 0) {
        parsed_request.op = Op::count;
    } else if (std::strcmp(cursor, "TEMP") == 0) {
        parsed_request.op = Op::temp;
    } else if (std::strcmp(cursor, "LED 0") == 0 || std::strcmp(cursor, "LED 1") == 0) {
        parsed_request.op = Op::led;
        parsed_request.led = cursor[4] == '1';
    } else {
        return false;
    }
    output_request = parsed_request;
    return true;
}
} // namespace hfs
