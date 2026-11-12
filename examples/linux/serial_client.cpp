#include "serial_port.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: serial_client DEVICE PING|COUNT|TEMP|LED [0|1]\n";
        return 2;
    }
    std::string command = argv[2];
    for (int argument_index = 3; argument_index < argc; ++argument_index) {
        command += " " + std::string(argv[argument_index]);
    }
    try {
        hfs::Serial serial(argv[1]);
        std::cout << serial.transact(1, command) << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
