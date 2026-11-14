cmake_minimum_required(VERSION 3.20)
project(hardware_from_software_linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

foreach(example IN ITEMS
    hello
    i2c_client
    serial_client
    gpio_events
    button_client
    memory_lab
    capstone
)
    add_executable(${example} ${example}.cpp)
    target_compile_options(${example} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()
