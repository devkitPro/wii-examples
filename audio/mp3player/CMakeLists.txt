cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

# Add a Mp3Player project and executable
message(STATUS "Adding Example : Mp3Player")
project(Mp3Player)
add_executable(Mp3Player.elf source/template.c)

# Add the mp3 binary into the executable so we can read it from the source
devkit_target_add_binary_sources(Mp3Player.elf ${CMAKE_CURRENT_LIST_DIR}/data/sample.mp3)

# Find and link libogc and the component inside libogc that lets us play mp3 files
find_package(libogc REQUIRED)
target_link_libraries(Mp3Player.elf PRIVATE libogc::libogc libogc::mad)
target_include_directories(Mp3Player.elf PRIVATE ${libogc_INCLUDE_DIRS})