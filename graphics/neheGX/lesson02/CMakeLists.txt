cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_02")

# Add a NeheGX_02 project and executable
project(NeheGX_02)
add_executable(NeheGX_02.elf source/lesson2.c)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_02.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_02.elf PRIVATE ${libogc_INCLUDE_DIRS})