#include <cerrno>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
    std::ifstream maps("/proc/self/maps");
    if(!maps){std::cerr<<"This lab expects Linux /proc\n";return 1;}
    std::cout<<maps.rdbuf();
    const long page=sysconf(_SC_PAGESIZE);
    if(page<=0)return 1;
    const std::size_t bytes=static_cast<std::size_t>(page)*128;
    void* region=mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(region==MAP_FAILED){std::cerr<<std::strerror(errno)<<'\n';return 1;}
    rusage before{},after{};
    if(getrusage(RUSAGE_SELF,&before)<0){munmap(region,bytes);return 1;}
    auto* data=static_cast<volatile unsigned char*>(region);
    for(std::size_t i=0;i<bytes;i+=static_cast<std::size_t>(page))data[i]=1;
    if(getrusage(RUSAGE_SELF,&after)<0){munmap(region,bytes);return 1;}
    std::cout<<"virtual mapping="<<region<<" page_size="<<page
             <<" minor_fault_delta="<<after.ru_minflt-before.ru_minflt<<'\n'<<std::flush;
    const pid_t child=fork();
    if(child<0){munmap(region,bytes);return 1;}
    if(child==0){
        rlimit limit{0,0};setrlimit(RLIMIT_CORE,&limit);
        if(mprotect(region,bytes,PROT_NONE)<0)_exit(2);
        data[0]=2; // Deliberate child-only protection fault.
        _exit(3);
    }
    int status=0;
    while(waitpid(child,&status,0)<0){if(errno==EINTR)continue;munmap(region,bytes);return 1;}
    const bool expected=WIFSIGNALED(status)&&WTERMSIG(status)==SIGSEGV;
    std::cout<<"child protection fault: "<<(expected?"expected SIGSEGV":"unexpected result")<<'\n';
    munmap(region,bytes);return expected?0:1;
}
