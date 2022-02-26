cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : USBKeyboard")

# Add a USBKeyboard project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(USBKeyboard)
add_executable(USBKeyboard.elf source/basic_stdin.c)

# Find and link to libogc and the special submodule of libogc 
# which allows for USBKeyboard
find_package(libogc REQUIRED)
target_link_libraries(USBKeyboard.elf PRIVATE libogc::libogc libogc::wiikeyboard)
target_include_directories(USBKeyboard.elf PRIVATE ${libogc_INCLUDE_DIRS})