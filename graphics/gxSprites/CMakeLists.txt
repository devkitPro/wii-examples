cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()
include(PPCUtilsAll)

message(STATUS "Adding Example : GXSprites")

# Add a GXSprites project and executable
project(GXSprites)
add_executable(GXSprites.elf source/gxsprites.c)

devkit_target_add_texture_sources(GXSprites.elf ${CMAKE_CURRENT_LIST_DIR}/textures/textures.scf)

# Find and link to libogc
find_package(libogc REQUIRED)
target_link_libraries(GXSprites.elf PRIVATE libogc::libogc)
target_include_directories(GXSprites.elf PRIVATE ${libogc_INCLUDE_DIRS})