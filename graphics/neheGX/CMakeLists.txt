cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

if(NOT ${CMAKE_DEVKITWII_FOUND})
	message(FATAL_ERROR "You have not provided the path to the WiiToolchain file")
endif()

add_subdirectory(lesson01)
add_subdirectory(lesson02)
add_subdirectory(lesson03)
add_subdirectory(lesson04)
add_subdirectory(lesson05)