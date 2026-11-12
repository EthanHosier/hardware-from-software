#include "protocol.hpp"
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    hfs::Request request;
    assert(hfs::parse("1 PING", request) && request.id == 1 &&
           request.op == hfs::Op::ping);
    assert(hfs::parse("999999 LED 1", request) && request.led);
    assert(hfs::parse("23 COUNT", request) && request.op == hfs::Op::count);
    assert(hfs::parse("24 TEMP", request) && request.op == hfs::Op::temp);
    for (auto invalid_request : {"", "0 PING", "1000000 PING", "1", "1 LED 2",
                                 "1 LED 1 x", "1  PING", "-1 PING", "x COUNT"}) {
        assert(!hfs::parse(invalid_request, request));
    }
    hfs::Line line;
    const std::string input = "2 LED 0\r\n3 COUNT\n";
    unsigned completed_line_count = 0;
    for (char byte : input) {
        if (line.feed(byte) == hfs::Line::Result::ready) {
            assert(hfs::parse(line.text(), request));
            ++completed_line_count;
        }
    }
    assert(completed_line_count == 2);
    for (int byte_index = 0; byte_index < 1000; ++byte_index) {
        assert(line.feed('X') == hfs::Line::Result::none);
    }
    assert(line.feed('\n') == hfs::Line::Result::overflow);
    for (char byte : std::string("4 PING\n")) {
        if (line.feed(byte) == hfs::Line::Result::ready) {
            assert(hfs::parse(line.text(), request) && request.id == 4);
        }
    }
    for (unsigned char invalid_byte : {0u, 1u, 9u, 127u, 255u}) {
        hfs::Line invalid_line;
        for (char byte : std::string("1 LED 1")) {
            invalid_line.feed(byte);
        }
        invalid_line.feed(static_cast<char>(invalid_byte));
        for (char byte : std::string("garbage")) {
            invalid_line.feed(byte);
        }
        assert(invalid_line.feed('\n') == hfs::Line::Result::overflow);
    }
    std::puts("protocol: success, malformed input, fragmented frames and overflow "
              "recovery passed");
}
