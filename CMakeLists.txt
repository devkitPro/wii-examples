cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# First check that the Wii or PPC toolchain is being used. 
# The Wii toolchain sets a variable in the cmake cache called 
# CMAKE_DEVKITWII_FOUND to true if the toolchain is found
# We won't usually be this verbose
if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(WARNING "Wii Toolchain not found yet! Attempt to include the submodule")
	include(${CMAKE_CURRENT_LIST_DIR}/DevkitPPC-CMake/DevkitWiiToolchain.cmake)
	if(NOT ${CMAKE_DEVKITWII_FOUND})
		message(FATAL_ERROR "Wii toolchain not being used, set -DCMAKE_TOOLCHAIN_FILE=\"PATH TO TOOLCHAIN\"")
	endif()
endif()

# Create an interface library that supports C and C++
# The interface property means that we don't generate an actual library
project(Wii-Examples VERSION 0.1.0 LANGUAGES CXX C)
add_library(Wii-Examples INTERFACE)

message(STATUS "Start building the wii examples")

# Add all of the cmake subdirectories one at a time
# This calls all of the lower cmake lists in those directories
add_subdirectory(template)