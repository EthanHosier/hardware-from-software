#include "protocol.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main() {
    hfs::Request r;
    assert(hfs::parse("1 PING", r) && r.id == 1 && r.op == hfs::Op::ping);
    assert(hfs::parse("999999 LED 1", r) && r.led);
    assert(hfs::parse("23 COUNT", r) && r.op == hfs::Op::count);
    assert(hfs::parse("24 TEMP", r) && r.op == hfs::Op::temp);
    for (auto s : {"", "0 PING", "1000000 PING", "1", "1 LED 2", "1 LED 1 x", "1  PING", "-1 PING", "x COUNT"})
        assert(!hfs::parse(s, r));
    hfs::Line line;
    const std::string input = "2 LED 0\r\n3 COUNT\n";
    unsigned ready = 0;
    for (char c : input) if (line.feed(c) == hfs::Line::Result::ready) {
        assert(hfs::parse(line.text(), r)); ++ready;
    }
    assert(ready == 2);
    for (int n = 0; n < 1000; ++n) assert(line.feed('X') == hfs::Line::Result::none);
    assert(line.feed('\n') == hfs::Line::Result::overflow);
    for (char c : std::string("4 PING\n")) if (line.feed(c) == hfs::Line::Result::ready)
        assert(hfs::parse(line.text(), r) && r.id == 4);
    for (unsigned char bad : {0u, 1u, 9u, 127u, 255u}) {
        hfs::Line invalid;
        for (char c : std::string("1 LED 1")) invalid.feed(c);
        invalid.feed(static_cast<char>(bad));
        for (char c : std::string("garbage")) invalid.feed(c);
        assert(invalid.feed('\n') == hfs::Line::Result::overflow);
    }
    std::puts("protocol: success, malformed input, fragmented frames and overflow recovery passed");
}
