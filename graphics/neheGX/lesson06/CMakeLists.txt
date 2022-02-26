cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_06")

# Add a NeheGX_06 project and executable
project(NeheGX_06)
add_executable(NeheGX_06.elf source/lesson6.c)

devkit_target_add_texture_sources(NeheGX_06.elf ${CMAKE_CURRENT_LIST_DIR}/textures/NeHe.scf)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_06.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_06.elf PRIVATE ${libogc_INCLUDE_DIRS})