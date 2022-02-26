cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# We're skipping this example for now since it requires 
# (dkp-)pacman -S ppc-libvorbisidec
# And that is another module file to add
return()

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

message(STATUS "Adding Example : OggPlayer")

# Add a OggPlater project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(OggPlayer)

add_executable(OggPlayer.elf 
	source/template.c
	source/oggplayer.c
	source/oggplayer.h)

devkit_target_add_binary_sources(OggPlayer.elf ${CMAKE_CURRENT_LIST_DIR}/data/sample.ogg)

# Find and link to libogc and the special submodule of libogc 
# which allows for modplay
find_package(libogc REQUIRED)
target_link_libraries(OggPlayer.elf PRIVATE libogc::libogc)
target_include_directories(OggPlayer.elf PRIVATE ${libogc_INCLUDE_DIRS})