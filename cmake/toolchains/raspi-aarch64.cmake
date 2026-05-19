set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

if(NOT DEFINED RASPI_SYSROOT)
    get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    set(RASPI_SYSROOT "${REPO_ROOT}/sysroot/raspi-aarch64")
endif()

set(CMAKE_SYSROOT "${RASPI_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH "${RASPI_SYSROOT}")

include_directories(SYSTEM
    "${RASPI_SYSROOT}/usr/include/aarch64-linux-gnu"
)

link_directories(
    "${RASPI_SYSROOT}/usr/lib/aarch64-linux-gnu"
    "${RASPI_SYSROOT}/lib/aarch64-linux-gnu"
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_SYSROOT_DIR} "${RASPI_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
    "${RASPI_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${RASPI_SYSROOT}/usr/lib/pkgconfig:${RASPI_SYSROOT}/usr/share/pkgconfig"
)
