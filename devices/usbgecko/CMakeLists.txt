cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : GDB_USBGecko")

# Add a GDB_USBGecko project and executable
project(GDB_USBGecko)
add_executable(GDB_USBGecko.elf source/gdbstub.c)

# Find and link to libogc debug library in libogc 
find_package(libogc REQUIRED)
target_link_libraries(GDB_USBGecko.elf PRIVATE libogc::libogc libogc::debug)
target_include_directories(GDB_USBGecko.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name GDB_USBGecko_emu with default settings
devkit_target_dolphin_auto(GDB_USBGecko.elf)