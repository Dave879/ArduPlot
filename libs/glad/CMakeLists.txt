cmake_minimum_required(VERSION 3.1.0)
project(glad VERSION 1.0 LANGUAGES C)

add_library(glad STATIC src/glad.c)

target_include_directories(glad
	PUBLIC
	$<INSTALL_INTERFACE:include>    
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)