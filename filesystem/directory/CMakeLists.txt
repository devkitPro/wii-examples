cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : Directory")

# Add a Directory project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(Directory)
add_executable(Directory.elf source/directory.c)

# Find and link to libogc and the special submodule of libogc 
# which allows for interfscing with the fat file system
find_package(libogc REQUIRED)
target_link_libraries(Directory.elf PRIVATE libogc::libogc libogc::fat)
target_include_directories(Directory.elf PRIVATE ${libogc_INCLUDE_DIRS})