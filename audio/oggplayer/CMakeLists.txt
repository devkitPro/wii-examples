cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : OggPlayer")

# We provide the user with an example on how to find the libraries installed in pacman
# The wii toolchain defines a list of search paths where devkit puts the libraries
# downloaded by pacman, it is called CMAKE_WII_SEARCH_PATHS
find_file(libogg NAMES "libogg.a" PATHS ${CMAKE_WII_SEARCH_PATHS})
find_file(libvorbisidec NAMES "libvorbisidec.a" PATHS ${CMAKE_WII_SEARCH_PATHS})

# If either of the above fails then they will have their containing variables set 
# to var-NOTFOUND, if that's the case, tell the user to download with pacman
if("${libogg}" STREQUAL "libogg-NOTFOUND" OR "${libvorbisidec}" STREQUAL "libvorbisidec-NOTFOUND")
	message(WARNING "Couldn't find vorbisidec in pacman, run (dkp-)pacman -S ppc-libvorbisidec")
	return()
endif()

# Now we can get the include directories of each of these libraries
get_filename_component(ogg_inc "${libogg}" DIRECTORY)
get_filename_component(vorbisidec_inc "${libvorbisidec}" DIRECTORY)
set(ogg_inc "${ogg_inc}/../include")
set(vorbisidec_inc "${vorbisidec_inc}/../include")

# Add a OggPlater project and executable
# The executable needs to compile the binary into the source tree
# We have a cmake helper file that will do that for us
project(OggPlayer)

add_executable(OggPlayer.elf 
	source/template.c
	source/oggplayer.c
	source/oggplayer.h)

devkit_target_add_binary_sources(OggPlayer.elf ${CMAKE_CURRENT_LIST_DIR}/data/sample.ogg)

# Find and link to libogc and the subcomponent of ogc which enables some audio
# Also make sure to link to the ogg libraries we found earlier
find_package(libogc REQUIRED)
target_link_libraries(OggPlayer.elf PRIVATE libogc::asnd ${libvorbisidec} ${libogg})

# Tell the executable where to find the include firectories for the ogg libraries
target_include_directories(OggPlayer.elf PRIVATE "${ogg_inc}" "${vorbisidec_inc}")

