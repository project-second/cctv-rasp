cmake_minimum_required(VERSION 3.16)

project(afterveda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(hardware-control)
add_subdirectory(media-server)
add_subdirectory(onvif-server)
