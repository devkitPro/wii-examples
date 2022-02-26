cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_12")

# Add a NeheGX_12 project and executable
project(NeheGX_12)
add_executable(NeheGX_12.elf source/lesson12.c)

devkit_target_add_texture_sources(NeheGX_12.elf ${CMAKE_CURRENT_LIST_DIR}/textures/Cube.scf)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_12.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_12.elf PRIVATE ${libogc_INCLUDE_DIRS})