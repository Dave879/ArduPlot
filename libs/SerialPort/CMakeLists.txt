cmake_minimum_required(VERSION 3.1.0)
project(SerialPort VERSION 1.0)

set(source_dir "${PROJECT_SOURCE_DIR}/src")
file(GLOB src_files "${source_dir}/*.cpp")

add_library(SerialPort STATIC ${src_files})

target_include_directories(SerialPort
	PUBLIC
	$<INSTALL_INTERFACE:include>    
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)