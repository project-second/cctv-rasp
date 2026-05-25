cmake_minimum_required(VERSION 3.16)

project(afterveda_onvif LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(afterveda-onvif
    src/app.cpp
    src/config.cpp
    src/discovery_server.cpp
    src/http_client.cpp
    src/http_server.cpp
    src/main.cpp
    src/ptz_service.cpp
    src/rail_client.cpp
    src/soap_services.cpp
    src/utils.cpp
)

target_link_libraries(afterveda-onvif
    PRIVATE
        Threads::Threads
)

target_compile_options(afterveda-onvif
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

install(TARGETS afterveda-onvif RUNTIME DESTINATION bin)
