cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : Directory")

# Add a Directory project and executable
project(Directory)
add_executable(Directory.elf source/directory.c)

# Find and link to libogc and the special submodule of libogc 
# which allows for interfacing with the fat file system
find_package(libogc REQUIRED)
target_link_libraries(Directory.elf PRIVATE libogc::libogc libogc::fat)
target_include_directories(Directory.elf PRIVATE ${libogc_INCLUDE_DIRS})

# Add target to run the executable with name Directory_emu with default settings
devkit_target_dolphin_auto(Directory.elf)