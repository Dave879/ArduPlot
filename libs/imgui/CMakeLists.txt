cmake_minimum_required(VERSION 3.1.0)
project(imgui VERSION 1.0 LANGUAGES C)

set(source_dir "${PROJECT_SOURCE_DIR}/src")
file(GLOB src_files "${source_dir}/*.cpp")

add_library(imgui STATIC ${src_files})

target_include_directories(imgui
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include>
	$<INSTALL_INTERFACE:include>    
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)