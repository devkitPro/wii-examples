cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_04")

# Add a NeheGX_04 project and executable
project(NeheGX_04)
add_executable(NeheGX_04.elf source/lesson4.c)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_04.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_04.elf PRIVATE ${libogc_INCLUDE_DIRS})