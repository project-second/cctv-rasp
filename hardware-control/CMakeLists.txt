cmake_minimum_required(VERSION 3.16)

project(rail_hardware_control LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hardware-control
    src/config.cpp
    src/main.cpp
    src/pan_tilt.cpp
    src/pca9685.cpp
    src/servo_controller.cpp
    src/state_store.cpp
)

target_compile_options(hardware-control
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

install(TARGETS hardware-control RUNTIME DESTINATION bin)
