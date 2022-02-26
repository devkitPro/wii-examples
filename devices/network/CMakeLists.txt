cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : Network")

# Add a Network project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(Network)
add_executable(Network.elf source/sockettest.c)

# Find and link to libogc and the special submodule of libogc 
# which allows for Network
find_package(libogc REQUIRED)
target_link_libraries(Network.elf PRIVATE libogc::libogc)
target_include_directories(Network.elf PRIVATE ${libogc_INCLUDE_DIRS})