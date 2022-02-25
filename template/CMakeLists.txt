cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# First check that the Wii or PPC toolchain is being used. 
# The Wii toolchain sets a variable in the cmake cache called 
# CMAKE_DEVKITWII_FOUND to true if the toolchain is found
# We won't usually be this verbose
if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(WARNING "Wii Toolchain not found yet! Attempt to include the submodule")
	include(${CMAKE_CURRENT_LIST_DIR}/../DevkitPPC-CMake/DevkitWiiToolchain.cmake)
	if(NOT ${CMAKE_DEVKITWII_FOUND})
		message(FATAL_ERROR "Wii toolchain not being used, set -DCMAKE_TOOLCHAIN_FILE=\"PATH TO TOOLCHAIN\"")
	endif()
endif()

message(STATUS "Adding Example : Template")

# Create a project and an executable inside that project 
# The executable is the end result of our build system
# For reference the executable needs to have a .elf 
# file extension so the compiler produces the right stuff
#
# We only have one source file inside template
# We ensure we specify all the sources local to this 
# CMakelist.txt file
project(Template)
add_executable(Template.elf source/template.c)

# libogc is the library provided by devkitPPC that allows us 
# to run code on the wii, and that's our main goal. It simplifies
# the process of starting the system and interfacing with hardware
#
# First we have to find libogc, we do this with find_package
# if the package was found it sets libogc_INCLUDE_DIRS
# and a series of different libraries, all namespaced inside
# libogc. So if you wanted to use wiiuse it would be
# libogc::wiiuse
#
# We link the libogc directories, so that our source files know where 
# find the header files for libogc, and then we link to libogc
# to get the actual implementation of those functions in our executable
find_package(libogc REQUIRED)
target_link_libraries(Template.elf PRIVATE libogc::libogc)
target_include_directories(Template.elf PRIVATE ${libogc_INCLUDE_DIRS})


