cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : Modplay")

# Add a modplay project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(Modplay)
add_executable(Modplay.elf source/template.c)
devkit_target_add_binary_sources(Modplay.elf ${CMAKE_CURRENT_LIST_DIR}/data/technique.mod)

# Find and link to libogc and the special submodule of libogc 
# which allows for modplay
find_package(libogc REQUIRED)
target_link_libraries(Modplay.elf PRIVATE libogc::libogc libogc::modplay)
target_include_directories(Modplay.elf PRIVATE ${libogc_INCLUDE_DIRS})