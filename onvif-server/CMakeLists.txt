cmake_minimum_required(VERSION 3.16)

project(afterveda_onvif LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

find_path(GSOAP_INCLUDE_DIR stdsoap2.h
    PATHS
        /usr/include
        /usr/include/gsoap
        /usr/local/include
        /usr/local/include/gsoap
)
find_library(GSOAP_LIBRARY
    NAMES gsoap++ gsoap
    PATHS
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
)
if (NOT GSOAP_INCLUDE_DIR OR NOT GSOAP_LIBRARY)
    message(FATAL_ERROR "gSOAP runtime not found. Install libgsoap-dev so stdsoap2.h and libgsoap++ are available.")
endif()

set(GSOAP_PLUGIN_SOURCES
    /usr/share/gsoap/plugin/wsddapi.c
    /usr/share/gsoap/plugin/wsaapi.c
    /usr/share/gsoap/custom/struct_timeval.c
)
set_source_files_properties(${GSOAP_PLUGIN_SOURCES} PROPERTIES LANGUAGE CXX)

add_executable(afterveda-onvif
    src/app.cpp
    src/config.cpp
    src/discovery_server.cpp
    src/http_client.cpp
    src/http_server.cpp
    src/main.cpp
    src/media_model.cpp
    src/media_xml.cpp
    src/rail_client.cpp
    src/soap_services.cpp
    src/utils.cpp
    gsoap/generated/soapC.cpp
    gsoap/generated/soapClient.cpp
    gsoap/generated/soapServer.cpp
    ${GSOAP_PLUGIN_SOURCES}
)

target_include_directories(afterveda-onvif
    PRIVATE
        ${GSOAP_INCLUDE_DIR}
        /usr/share/gsoap
        /usr/share/gsoap/import
        /usr/share/gsoap/plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/gsoap/generated
)

target_link_libraries(afterveda-onvif
    PRIVATE
        ${GSOAP_LIBRARY}
        Threads::Threads
)

target_compile_options(afterveda-onvif
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

install(TARGETS afterveda-onvif RUNTIME DESTINATION bin)
