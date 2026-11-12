#include "xsk_common.hpp"

int main(int argc, char** argv) {
    try {
        return hfs::net::run_xsk(argc, argv, true);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
