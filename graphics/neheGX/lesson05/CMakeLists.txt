cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_05")

# Add a NeheGX_05 project and executable
project(NeheGX_05)
add_executable(NeheGX_05.elf source/lesson5.c)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_05.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_05.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name NeheGX_05_emu with default settings
devkit_target_dolphin_auto(NeheGX_05.elf)