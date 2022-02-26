cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_08")

# Add a NeheGX_08 project and executable
project(NeheGX_08)
add_executable(NeheGX_08.elf source/lesson8.c)

devkit_target_add_texture_sources(NeheGX_08.elf ${CMAKE_CURRENT_LIST_DIR}/textures/glass.scf)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_08.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_08.elf PRIVATE ${libogc_INCLUDE_DIRS})