#include "serial_port.hpp"
#include <charconv>
#include <iostream>
#include <string>
int main(int argc,char** argv) {
    if(argc!=4){std::cerr<<"usage: capstone /dev/mybutton /dev/serial0 EVENT_COUNT\n";return 2;}
    unsigned count=0;std::string arg=argv[3];
    auto [end,ec]=std::from_chars(arg.data(),arg.data()+arg.size(),count);
    if(ec!=std::errc{}||end!=arg.data()+arg.size()||!count||count>1000)return 2;
    const int fd=open(argv[1],O_RDONLY|O_NONBLOCK|O_CLOEXEC);
    if(fd<0){std::cerr<<std::strerror(errno)<<'\n';return 1;}
    try {
        hfs::Serial serial(argv[2]);
        for(unsigned i=1;i<=count;){
            pollfd p{fd,POLLIN,0};int n=poll(&p,1,30000);
            if(n<0 && errno==EINTR)continue;
            if(n<=0 || (p.revents&(POLLERR|POLLHUP|POLLNVAL)))throw std::runtime_error("button timeout/offline");
            if(!(p.revents&POLLIN))continue;
            char bytes[64];const auto got=read(fd,bytes,sizeof(bytes));
            if(got<0 && (errno==EAGAIN||errno==EINTR))continue;
            if(got<=0)throw std::runtime_error("button read failed");
            const std::string record(bytes,static_cast<std::size_t>(got));
            const auto sep=record.find(' ');
            if(sep==std::string::npos || record.size()!=sep+3 || record.back()!='\n' ||
               (record[sep+1]!='0' && record[sep+1]!='1'))throw std::runtime_error("bad button record");
            const std::string command=std::string("LED ")+record[sep+1];
            const auto reply=serial.transact(i,command);
            if(reply!=std::to_string(i)+" OK "+record[sep+1])throw std::runtime_error("LED acknowledgement mismatch");
            std::cout<<"button "<<record.substr(0,sep)<<" -> "<<reply<<'\n';++i;
        }
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';close(fd);return 1;}
    close(fd);
}
