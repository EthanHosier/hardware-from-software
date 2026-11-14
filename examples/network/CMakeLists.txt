cmake_minimum_required(VERSION 3.20)
project(hardware_from_software_network LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

foreach(example IN ITEMS wire_test udp_endpoint packet_rx packet_ring)
    add_executable(${example} ${example}.cpp)
    target_compile_options(${example} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

# A local cross compiler can check XDP sources and provide IDE navigation
# without having target-architecture libbpf/libxdp libraries to link against.
option(HFS_NETWORK_XDP_COMPILE_ONLY "Compile XDP clients without linking" OFF)
set(HFS_NETWORK_HEADERS "" CACHE PATH "Directory containing bpf/ and xdp/ headers")

if(HFS_NETWORK_XDP_COMPILE_ONLY)
    find_path(HFS_BPF_INCLUDE_DIR bpf/libbpf.h HINTS "${HFS_NETWORK_HEADERS}" REQUIRED)
    find_path(HFS_XDP_INCLUDE_DIR xdp/xsk.h HINTS "${HFS_NETWORK_HEADERS}" REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NETWORK_XDP REQUIRED IMPORTED_TARGET libxdp libbpf)
endif()

foreach(example IN ITEMS xdp_control xsk_receiver xsk_responder)
    if(HFS_NETWORK_XDP_COMPILE_ONLY)
        add_library(${example} OBJECT ${example}.cpp)
        target_include_directories(${example} SYSTEM PRIVATE
            "${HFS_BPF_INCLUDE_DIR}" "${HFS_XDP_INCLUDE_DIR}")
    else()
        add_executable(${example} ${example}.cpp)
        target_link_libraries(${example} PRIVATE PkgConfig::NETWORK_XDP)
    endif()
    target_compile_options(${example} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

# Clang-compatible cross compilers can give the BPF source its own target,
# keeping both the object and the IDE's compilation database accurate.
set(HFS_BPF_TARGET "bpf" CACHE STRING "Clang target for the BPF program")
set(HFS_BPF_SYSTEM_INCLUDE_DIRS "" CACHE STRING "Additional BPF system include directories")
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_library(lab_xdp OBJECT lab_xdp.bpf.c)
    target_compile_options(lab_xdp PRIVATE -target "${HFS_BPF_TARGET}" -O2 -g -Wall -Wextra)
    # Retargeting changes Clang's implicit includes. Pass these explicitly:
    # CMake otherwise removes paths already implicit for the Linux compiler.
    foreach(include_dir IN LISTS HFS_BPF_SYSTEM_INCLUDE_DIRS)
        target_compile_options(lab_xdp PRIVATE "-isystem${include_dir}")
    endforeach()
    if(HFS_NETWORK_XDP_COMPILE_ONLY)
        target_include_directories(lab_xdp SYSTEM PRIVATE "${HFS_BPF_INCLUDE_DIR}")
    else()
        target_link_libraries(lab_xdp PRIVATE PkgConfig::NETWORK_XDP)
        target_include_directories(lab_xdp SYSTEM PRIVATE "/usr/include/${CMAKE_LIBRARY_ARCHITECTURE}")
    endif()
else()
    find_program(HFS_BPF_CLANG NAMES clang REQUIRED)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lab_xdp.bpf.o"
        COMMAND "${HFS_BPF_CLANG}" -target bpf -O2 -g -Wall -Wextra
            -I "/usr/include/${CMAKE_LIBRARY_ARCHITECTURE}"
            ${NETWORK_XDP_CFLAGS} -c "${CMAKE_CURRENT_SOURCE_DIR}/lab_xdp.bpf.c"
            -o "${CMAKE_CURRENT_BINARY_DIR}/lab_xdp.bpf.o"
        DEPENDS lab_xdp.bpf.c VERBATIM)
    add_custom_target(lab_xdp ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lab_xdp.bpf.o"
        SOURCES lab_xdp.bpf.c)
endif()
