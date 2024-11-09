cmake_minimum_required(VERSION 3.13)

project(application LANGUAGES C VERSION 1.0.0)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
	source/main.c
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

target_include_directories(${PROJECT_NAME} PRIVATE
  include
)

ogc_create_dol(${PROJECT_NAME})

