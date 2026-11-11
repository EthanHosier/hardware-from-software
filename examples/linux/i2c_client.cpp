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
int main(int argc,char** argv) {
    const int fd=open(argc>1?argv[1]:"/dev/i2c-3",O_RDWR|O_CLOEXEC);
    if(fd<0) { const int error=errno; std::cerr<<"open: "<<std::strerror(error)<<'\n'; return 1; }
    try {
        unsigned long functions=0;
        if(ioctl(fd,I2C_FUNCS,&functions)<0) {
            const int error=errno;
            throw std::system_error(error,std::generic_category(),"query adapter");
        }
        if(!(functions&I2C_FUNC_I2C))
            throw std::runtime_error("adapter lacks combined I2C transfers");
        uint8_t reg=0;
        std::array<uint8_t,16> data{};
        i2c_msg msgs[]{{0x42,0,1,&reg},{0x42,I2C_M_RD,static_cast<__u16>(data.size()),data.data()}};
        i2c_rdwr_ioctl_data request{msgs,2};
        const int read_result=ioctl(fd,I2C_RDWR,&request);
        if(read_result<0) {
            const int error=errno;
            throw std::system_error(error,std::generic_category(),"combined read");
        }
        if(read_result!=2) throw std::runtime_error("incomplete combined read");
        if(data[0]!=0x42 || data[1]!=1) throw std::runtime_error("unexpected identity/protocol");
        auto u32=[&](unsigned at){ uint32_t n=0; for(unsigned i=0;i<4;++i)n|=uint32_t(data[at+i])<<(8*i); return n; };
        const uint32_t bits=u32(8);
        const int64_t temp=bits<0x80000000u?bits:static_cast<int64_t>(bits)-0x100000000LL;
        std::cout<<"identity=0x42 protocol=1 led="<<unsigned(data[2])<<" presses="<<u32(4)
                 <<" chip_milliC="<<temp<<'\n';
        uint8_t write_data[]{2,static_cast<uint8_t>(data[2]?0:1)};
        i2c_msg out{0x42,0,2,write_data}; i2c_rdwr_ioctl_data wr{&out,1};
        const int write_result=ioctl(fd,I2C_RDWR,&wr);
        if(write_result<0) {
            const int error=errno;
            throw std::system_error(error,std::generic_category(),"LED write");
        }
        if(write_result!=1) throw std::runtime_error("incomplete LED write");
        std::cout<<"LED update requested; run again to read a fresh snapshot\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; close(fd); return 1; }
    close(fd);
}
