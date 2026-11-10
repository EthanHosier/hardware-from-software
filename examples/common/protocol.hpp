#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hfs {
// Fixed storage; feed one byte at a time. Overlong input is discarded to LF.
class Line {
    char data_[64]{};
    std::size_t used_ = 0;
    bool dropping_ = false;
public:
    enum class Result { none, ready, overflow };
    const char* text() const { return data_; }
    Result feed(char c) {
        if (c == '\r') return Result::none;
        if (c == '\n') {
            if (dropping_) { dropping_ = false; used_ = 0; return Result::overflow; }
            data_[used_] = '\0'; used_ = 0; return Result::ready;
        }
        if (dropping_) return Result::none;
        if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
            dropping_ = true; used_ = 0; return Result::none;
        }
        if (used_ == sizeof(data_) - 1) { dropping_ = true; used_ = 0; return Result::none; }
        data_[used_++] = c;
        return Result::none;
    }
};
enum class Op { ping, led, count, temp };
struct Request { std::uint32_t id = 0; Op op = Op::ping; bool led = false; };
inline bool parse(const char* line, Request& out) {
    const char* p = line;
    std::uint32_t id = 0;
    unsigned digits = 0;
    while (*p >= '0' && *p <= '9') {
        if (++digits > 6) return false;
        id = id * 10u + static_cast<unsigned>(*p++ - '0');
    }
    if (!id || *p++ != ' ') return false;
    Request r; r.id = id;
    if (std::strcmp(p, "PING") == 0) r.op = Op::ping;
    else if (std::strcmp(p, "COUNT") == 0) r.op = Op::count;
    else if (std::strcmp(p, "TEMP") == 0) r.op = Op::temp;
    else if (std::strcmp(p, "LED 0") == 0 || std::strcmp(p, "LED 1") == 0) {
        r.op = Op::led; r.led = p[4] == '1';
    } else return false;
    out = r;
    return true;
}
}
