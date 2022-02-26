cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : GDB_USBGecko")

# Add a GDB_USBGecko project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(GDB_USBGecko)
add_executable(GDB_USBGecko.elf source/gdbstub.c)

# Find and link to libogc and the special submodule of libogc 
# which allows for GDB_USBGecko
find_package(libogc REQUIRED)
target_link_libraries(GDB_USBGecko.elf PRIVATE libogc::libogc libogc::debug)
target_include_directories(GDB_USBGecko.elf PRIVATE ${libogc_INCLUDE_DIRS})