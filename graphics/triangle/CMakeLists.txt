cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : Triangle")

# Add a Triangle project and executable
project(Triangle)
add_executable(Triangle.elf source/triangle.c)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(Triangle.elf PRIVATE libogc::libogc)
target_include_directories(Triangle.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name Triangle_emu with default settings
devkit_target_dolphin_auto(Triangle.elf)