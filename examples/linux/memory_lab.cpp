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
    std::ifstream process_maps("/proc/self/maps");
    if (!process_maps) {
        std::cerr << "This lab expects Linux /proc\n";
        return 1;
    }
    std::cout << process_maps.rdbuf();
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return 1;
    }
    const std::size_t mapping_size = static_cast<std::size_t>(page_size) * 128;
    void* mapping = mmap(nullptr, mapping_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED) {
        std::cerr << std::strerror(errno) << '\n';
        return 1;
    }
    rusage usage_before{}, usage_after{};
    if (getrusage(RUSAGE_SELF, &usage_before) < 0) {
        munmap(mapping, mapping_size);
        return 1;
    }
    auto* mapped_bytes = static_cast<volatile unsigned char*>(mapping);
    for (std::size_t page_offset = 0; page_offset < mapping_size;
         page_offset += static_cast<std::size_t>(page_size)) {
        mapped_bytes[page_offset] = 1;
    }
    if (getrusage(RUSAGE_SELF, &usage_after) < 0) {
        munmap(mapping, mapping_size);
        return 1;
    }
    std::cout << "virtual mapping=" << mapping << " page_size=" << page_size
              << " minor_fault_delta=" << usage_after.ru_minflt - usage_before.ru_minflt
              << '\n'
              << std::flush;
    const pid_t child_pid = fork();
    if (child_pid < 0) {
        munmap(mapping, mapping_size);
        return 1;
    }
    if (child_pid == 0) {
        rlimit core_limit{0, 0};
        setrlimit(RLIMIT_CORE, &core_limit);
        if (mprotect(mapping, mapping_size, PROT_NONE) < 0) {
            _exit(2);
        }
        mapped_bytes[0] = 2; // Deliberate child-only protection fault.
        _exit(3);
    }
    int child_status = 0;
    while (waitpid(child_pid, &child_status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        munmap(mapping, mapping_size);
        return 1;
    }
    const bool expected_fault =
        WIFSIGNALED(child_status) && WTERMSIG(child_status) == SIGSEGV;
    std::cout << "child protection fault: "
              << (expected_fault ? "expected SIGSEGV" : "unexpected result") << '\n';
    munmap(mapping, mapping_size);
    return expected_fault ? 0 : 1;
}
