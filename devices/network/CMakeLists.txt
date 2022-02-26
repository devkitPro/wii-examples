cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : Network")

# Add a Network project and executable
project(Network)
add_executable(Network.elf source/sockettest.c)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(Network.elf PRIVATE libogc::libogc)
target_include_directories(Network.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name Network_emu with default settings
devkit_target_dolphin_auto(Network.elf)