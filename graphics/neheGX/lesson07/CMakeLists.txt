cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : NeheGX_07")

# Add a NeheGX_07 project and executable
project(NeheGX_07)
add_executable(NeheGX_07.elf source/lesson7.c)

devkit_target_add_texture_sources(NeheGX_07.elf ${CMAKE_CURRENT_LIST_DIR}/textures/crate.scf)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(NeheGX_07.elf PRIVATE libogc::libogc)
target_include_directories(NeheGX_07.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name NeheGX_07_emu with default settings
devkit_target_dolphin_auto(NeheGX_07.elf)