cmake_minimum_required(VERSION 3.16)

project(rail_media LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_RTSP_SERVER REQUIRED IMPORTED_TARGET gstreamer-rtsp-server-1.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

add_executable(rail-media
    src/config.cpp
    src/control_server.cpp
    src/main.cpp
    src/media_server.cpp
    src/pipeline.cpp
    src/profiles.cpp
)

target_link_libraries(rail-media
    PRIVATE
        PkgConfig::GSTREAMER
        PkgConfig::GST_RTSP_SERVER
        PkgConfig::GIO
)

target_compile_options(rail-media
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

install(TARGETS rail-media RUNTIME DESTINATION bin)
