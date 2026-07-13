cmake_minimum_required(VERSION 3.16)

project(afterveda_onvif LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(GSOAP_LIBRARY_ARCH_PATHS
    lib
    lib/${CMAKE_LIBRARY_ARCHITECTURE}
    lib/aarch64-linux-gnu
    lib/arm-linux-gnueabihf
    lib/x86_64-linux-gnu
)

find_path(GSOAP_INCLUDE_DIR stdsoap2.h
    PATH_SUFFIXES
        include
        include/gsoap
)
find_library(GSOAP_LIBRARY
    NAMES gsoap++ gsoap
    PATH_SUFFIXES
        ${GSOAP_LIBRARY_ARCH_PATHS}
)
if (NOT GSOAP_INCLUDE_DIR OR NOT GSOAP_LIBRARY)
    message(FATAL_ERROR
        "gSOAP runtime not found. Install libgsoap-dev on the target Raspberry Pi "
        "and sync /usr/include and /usr/lib into the cross sysroot, or install "
        "libgsoap-dev locally for native builds."
    )
endif()

file(READ "${GSOAP_INCLUDE_DIR}/stdsoap2.h" GSOAP_HEADER_TEXT)
string(REGEX MATCH "#define[ \t]+GSOAP_VERSION[ \t]+([0-9]+)" _GSOAP_HEADER_VERSION_MATCH "${GSOAP_HEADER_TEXT}")
set(GSOAP_HEADER_VERSION "${CMAKE_MATCH_1}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/gsoap/generated/soapStub.h" GSOAP_STUB_TEXT)
string(REGEX MATCH "GSOAP VERSION[ \t]+([0-9]+)[ \t]+MISMATCH" _GSOAP_STUB_VERSION_MATCH "${GSOAP_STUB_TEXT}")
set(GSOAP_STUB_VERSION "${CMAKE_MATCH_1}")
if (GSOAP_HEADER_VERSION AND GSOAP_STUB_VERSION AND NOT GSOAP_HEADER_VERSION STREQUAL GSOAP_STUB_VERSION)
    message(FATAL_ERROR
        "gSOAP generated code version ${GSOAP_STUB_VERSION} does not match "
        "the selected gSOAP runtime/header version ${GSOAP_HEADER_VERSION}. "
        "Regenerate onvif-server/gsoap/generated with the same soapcpp2 version "
        "as the target runtime, then configure again."
    )
endif()

find_path(GSOAP_SHARE_DIR
    NAMES plugin/wsddapi.c plugin/wsaapi.c custom/struct_timeval.c
    PATHS
        "${CMAKE_SYSROOT}/usr/share/gsoap"
        /usr/share/gsoap
        /usr/local/share/gsoap
    NO_DEFAULT_PATH
)
if (NOT GSOAP_SHARE_DIR)
    message(FATAL_ERROR
        "gSOAP source directory not found. Install gsoap on the build machine "
        "or sync /usr/share/gsoap into the Raspberry Pi sysroot."
    )
endif()

set(GSOAP_PLUGIN_SOURCES
    ${GSOAP_SHARE_DIR}/plugin/wsddapi.c
    ${GSOAP_SHARE_DIR}/plugin/wsaapi.c
    ${GSOAP_SHARE_DIR}/custom/struct_timeval.c
)
set_source_files_properties(${GSOAP_PLUGIN_SOURCES} PROPERTIES LANGUAGE CXX)

add_executable(afterveda-onvif
    src/core/app.cpp
    src/core/config.cpp
    src/core/utils.cpp
    src/media/media_model.cpp
    src/network/discovery_server.cpp
    src/network/http_client.cpp
    src/network/http_server.cpp
    src/network/rail_client.cpp
    src/ptz/ptz_device.cpp
    src/soap/soap_common.cpp
    src/soap/soap_device_service.cpp
    src/soap/soap_fault_service.cpp
    src/soap/soap_imaging_service.cpp
    src/soap/soap_media_service.cpp
    src/soap/soap_osd_service.cpp
    src/soap/soap_ptz_service.cpp
    src/main.cpp
    gsoap/generated/soapC.cpp
    gsoap/generated/soapClient.cpp
    gsoap/generated/soapServer.cpp
    ${GSOAP_PLUGIN_SOURCES}
)

target_include_directories(afterveda-onvif
    PRIVATE
        ${GSOAP_INCLUDE_DIR}
        ${GSOAP_SHARE_DIR}
        ${GSOAP_SHARE_DIR}/import
        ${GSOAP_SHARE_DIR}/plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/media
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ptz
        ${CMAKE_CURRENT_SOURCE_DIR}/src/soap
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
