#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdexcept>
#include <system_error>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char** argv) {
    const int adapter_fd = open(argc > 1 ? argv[1] : "/dev/i2c-3", O_RDWR | O_CLOEXEC);
    if (adapter_fd < 0) {
        const int error = errno;
        std::cerr << "open: " << std::strerror(error) << '\n';
        return 1;
    }
    try {
        unsigned long adapter_functions = 0;
        if (ioctl(adapter_fd, I2C_FUNCS, &adapter_functions) < 0) {
            const int error = errno;
            throw std::system_error(error, std::generic_category(), "query adapter");
        }
        if (!(adapter_functions & I2C_FUNC_I2C)) {
            throw std::runtime_error("adapter lacks combined I2C transfers");
        }
        uint8_t register_address = 0;
        std::array<uint8_t, 16> registers{};
        i2c_msg messages[]{
            {0x42, 0, 1, &register_address},
            {0x42, I2C_M_RD, static_cast<__u16>(registers.size()), registers.data()}};
        i2c_rdwr_ioctl_data request{messages, 2};
        const int read_result = ioctl(adapter_fd, I2C_RDWR, &request);
        if (read_result < 0) {
            const int error = errno;
            throw std::system_error(error, std::generic_category(), "combined read");
        }
        if (read_result != 2) {
            throw std::runtime_error("incomplete combined read");
        }
        if (registers[0] != 0x42 || registers[1] != 1) {
            throw std::runtime_error("unexpected identity/protocol");
        }
        auto read_le32 = [&](unsigned offset) {
            uint32_t value = 0;
            for (unsigned byte_index = 0; byte_index < 4; ++byte_index) {
                value |= uint32_t(registers[offset + byte_index]) << (8 * byte_index);
            }
            return value;
        };
        const uint32_t temperature_bits = read_le32(8);
        const int64_t temperature_millicelsius =
            temperature_bits < 0x80000000u
                ? temperature_bits
                : static_cast<int64_t>(temperature_bits) - 0x100000000LL;
        std::cout << "identity=0x42 protocol=1 led=" << unsigned(registers[2])
                  << " presses=" << read_le32(4)
                  << " chip_milliC=" << temperature_millicelsius << '\n';
        uint8_t write_data[]{2, static_cast<uint8_t>(registers[2] ? 0 : 1)};
        i2c_msg led_message{0x42, 0, 2, write_data};
        i2c_rdwr_ioctl_data write_request{&led_message, 1};
        const int write_result = ioctl(adapter_fd, I2C_RDWR, &write_request);
        if (write_result < 0) {
            const int error = errno;
            throw std::system_error(error, std::generic_category(), "LED write");
        }
        if (write_result != 1) {
            throw std::runtime_error("incomplete LED write");
        }
        std::cout << "LED update requested; run again to read a fresh snapshot\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        close(adapter_fd);
        return 1;
    }
    close(adapter_fd);
}
