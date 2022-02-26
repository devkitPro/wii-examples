cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : USBKeyboard")

# Add a USBKeyboard project and executable
project(USBKeyboard)
add_executable(USBKeyboard.elf source/basic_stdin.c)

# Find and link to libogc and usb keyboard library in libogc
find_package(libogc REQUIRED)
target_link_libraries(USBKeyboard.elf PRIVATE libogc::libogc libogc::wiikeyboard)
target_include_directories(USBKeyboard.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name USBKeyboard_emu with default settings
devkit_target_dolphin_auto(USBKeyboard.elf)