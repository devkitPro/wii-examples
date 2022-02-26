cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_03")

# Add a NeheGX_03 project and executable
project(NeheGX_03)
add_executable(NeheGX_03.elf source/lesson3.c)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_03.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_03.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name NeheGX_03_emu with default settings
devkit_target_dolphin_auto(NeheGX_03.elf)