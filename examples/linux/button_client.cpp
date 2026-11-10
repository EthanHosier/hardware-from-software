#include <cerrno>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <unistd.h>
int main(int argc,char** argv) {
    if(argc!=3){std::cerr<<"usage: button_client /dev/mybutton EVENT_COUNT\n";return 2;}
    unsigned count=0;const std::string arg=argv[2];
    auto [end,ec]=std::from_chars(arg.data(),arg.data()+arg.size(),count);
    if(ec!=std::errc{} || end!=arg.data()+arg.size() || count==0 || count>1000)return 2;
    int fd=open(argv[1],O_RDONLY|O_NONBLOCK|O_CLOEXEC);
    if(fd<0){std::cerr<<std::strerror(errno)<<'\n';return 1;}
    unsigned seen=0;
    while(seen<count){
        pollfd p{fd,POLLIN,0};int n=poll(&p,1,30000);
        if(n<0 && errno==EINTR)continue;
        if(n<=0 || (p.revents&(POLLERR|POLLHUP|POLLNVAL))){std::cerr<<"timeout, error, or device offline\n";close(fd);return 1;}
        if(!(p.revents&POLLIN))continue;
        char buffer[64];auto len=read(fd,buffer,sizeof(buffer));
        if(len<0 && (errno==EINTR || errno==EAGAIN))continue;
        if(len<=0){std::cerr<<"read: "<<std::strerror(errno)<<'\n';close(fd);return 1;}
        std::cout.write(buffer,len);++seen;
    }
    close(fd);
}
