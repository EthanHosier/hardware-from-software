#pragma once
#include <cerrno>
#include <chrono>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>
#include "../common/protocol.hpp"
namespace hfs {
class Serial {
    int fd_=-1;
    using Clock=std::chrono::steady_clock;
    void ready(short events,Clock::time_point end) {
        while(true) {
            const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(end-Clock::now()).count();
            if(ms<=0) throw std::runtime_error("UART deadline expired");
            pollfd p{fd_,events,0};
            const int n=poll(&p,1,static_cast<int>(ms));
            if(n<0 && errno==EINTR) continue;
            if(n<0) throw std::runtime_error(std::strerror(errno));
            if(n==0) throw std::runtime_error("UART deadline expired");
            if(p.revents & (POLLERR|POLLHUP|POLLNVAL)) throw std::runtime_error("UART disconnected");
            if(p.revents & events) return;
        }
    }
public:
    explicit Serial(const char* path) {
        fd_=open(path,O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC);
        if(fd_<0) throw std::runtime_error(std::string("open UART: ")+std::strerror(errno));
        termios t{};
        if(tcgetattr(fd_,&t)<0) { close(fd_); fd_=-1; throw std::runtime_error("tcgetattr failed"); }
        cfmakeraw(&t);
        t.c_iflag &= ~(IXON | IXOFF | IXANY);
        cfsetispeed(&t,B115200); cfsetospeed(&t,B115200);
        t.c_cflag=(t.c_cflag & ~(CSIZE|PARENB|CSTOPB|CRTSCTS))|CS8|CLOCAL|CREAD;
        t.c_cc[VMIN]=0; t.c_cc[VTIME]=0;
        if(tcsetattr(fd_,TCSANOW,&t)<0 || tcflush(fd_,TCIOFLUSH)<0) {
            close(fd_); fd_=-1; throw std::runtime_error("UART configuration failed");
        }
    }
    ~Serial(){ if(fd_>=0)close(fd_); }
    Serial(const Serial&)=delete; Serial& operator=(const Serial&)=delete;
    std::string transact(unsigned id,const std::string& command) {
        const std::string request=std::to_string(id)+" "+command;
        Request parsed;
        if(request.size()>63 || !parse(request.c_str(),parsed)) throw std::runtime_error("invalid command");
        const auto end=Clock::now()+std::chrono::seconds(2);
        const std::string bytes=request+"\n";
        std::size_t sent=0;
        while(sent<bytes.size()) {
            ready(POLLOUT,end);
            const auto n=write(fd_,bytes.data()+sent,bytes.size()-sent);
            if(n<0 && (errno==EINTR || errno==EAGAIN))continue;
            if(n<=0)throw std::runtime_error("UART write failed");
            sent+=static_cast<std::size_t>(n);
        }
        Line line;
        while(true) {
            ready(POLLIN,end);
            char c; const auto n=read(fd_,&c,1);
            if(n<0 && (errno==EINTR || errno==EAGAIN))continue;
            if(n<=0)throw std::runtime_error("UART read failed");
            const auto result=line.feed(c);
            if(result==Line::Result::overflow)throw std::runtime_error("overlong reply");
            if(result==Line::Result::ready) {
                const std::string reply=line.text();
                const std::string prefix=std::to_string(id)+" ";
                if(reply.compare(0,prefix.size(),prefix)!=0)throw std::runtime_error("unexpected reply ID");
                if(reply.compare(prefix.size(),3,"OK ")!=0)throw std::runtime_error("device rejected command: "+reply);
                const auto start=prefix.size()+3;
                long long value=0;
                const auto [finish,error]=std::from_chars(reply.data()+start,reply.data()+reply.size(),value);
                if(error!=std::errc{} || finish!=reply.data()+reply.size()) throw std::runtime_error("malformed numeric reply");
                if(parsed.op==Op::ping && value!=1) throw std::runtime_error("unexpected PING result");
                if(parsed.op==Op::led && value!=parsed.led) throw std::runtime_error("LED acknowledgement mismatch");
                if(parsed.op==Op::count && (value<0 || value>4294967295LL)) throw std::runtime_error("COUNT outside uint32 range");
                if(parsed.op==Op::temp && (value<INT32_MIN || value>INT32_MAX)) throw std::runtime_error("TEMP outside int32 range");
                return reply;
            }
        }
    }
};
}
