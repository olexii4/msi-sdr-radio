cmake_minimum_required(VERSION 3.10)
project(msi-sdr-radio C)

set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

########################################################################
# Find dependencies
########################################################################
find_package(PkgConfig REQUIRED)

find_package(LibUSB)
if(NOT LIBUSB_FOUND)
    message(FATAL_ERROR "LibUSB 1.0 required to compile MSi-SDR")
endif()

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(FFTW3F REQUIRED fftw3f)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

########################################################################
# Kernel driver detach option
########################################################################
option(DETACH_KERNEL_DRIVER "Detach kernel driver if loaded" OFF)
if(DETACH_KERNEL_DRIVER)
    message(STATUS "Building with kernel driver detaching enabled")
    add_definitions(-DDETACH_KERNEL_DRIVER=1)
endif()

########################################################################
# libmirisdr (static library)
########################################################################
add_library(mirisdr STATIC
    lib/mirisdr/src/libmirisdr.c
)

target_include_directories(mirisdr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mirisdr/include
    ${LIBUSB_INCLUDE_DIR}
)

target_link_libraries(mirisdr
    ${LIBUSB_LIBRARIES}
)

if(CMAKE_COMPILER_IS_GNUCC AND NOT WIN32)
    target_compile_options(mirisdr PRIVATE -Wall -Wextra -Wno-unused -Wsign-compare)
endif()

########################################################################
# MSi-SDR executable
########################################################################
add_executable(msi-sdr
    src/main.c
    src/device.c
    src/dsp.c
    src/audio.c
    src/ui.c
)

target_include_directories(msi-sdr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mirisdr/include
    ${LIBUSB_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
)

target_link_directories(msi-sdr PRIVATE
    ${SDL2_LIBRARY_DIRS}
    ${FFTW3F_LIBRARY_DIRS}
)

target_link_libraries(msi-sdr
    mirisdr
    ${LIBUSB_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${FFTW3F_LIBRARIES}
    OpenGL::GL
    Threads::Threads
    m
)

target_compile_definitions(msi-sdr PRIVATE
    GL_SILENCE_DEPRECATION
)
