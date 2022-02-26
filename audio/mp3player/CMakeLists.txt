cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : Mp3Player")

# Add a modplay project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(Mp3Player)
add_executable(Mp3Player.elf source/template.c)
devkit_target_add_binary_sources(Mp3Player.elf ${CMAKE_CURRENT_LIST_DIR}/data/sample.mp3)

# Find and link to libogc and the special submodule of libogc 
# which allows for modplay
find_package(libogc REQUIRED)
target_link_libraries(Mp3Player.elf PRIVATE libogc::libogc libogc::mad)
target_include_directories(Mp3Player.elf PRIVATE ${libogc_INCLUDE_DIRS})