#include "serial_port.hpp"
#include <iostream>
int main(int argc,char** argv) {
    if(argc<3) { std::cerr<<"usage: serial_client DEVICE PING|COUNT|TEMP|LED [0|1]\n"; return 2; }
    std::string command=argv[2];
    for(int i=3;i<argc;++i)command+=" "+std::string(argv[i]);
    try { hfs::Serial serial(argv[1]); std::cout<<serial.transact(1,command)<<'\n'; }
    catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
