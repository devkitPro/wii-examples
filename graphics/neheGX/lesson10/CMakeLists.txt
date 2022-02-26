cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_10")

# Add a NeheGX_10 project and executable
project(NeheGX_10)
add_executable(NeheGX_10.elf source/lesson10.c)

devkit_target_add_texture_sources(NeheGX_10.elf ${CMAKE_CURRENT_LIST_DIR}/textures/mud.scf)
devkit_target_add_binary_sources(NeheGX_10.elf ${CMAKE_CURRENT_LIST_DIR}/data/world.txt)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_10.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_10.elf PRIVATE ${libogc_INCLUDE_DIRS})