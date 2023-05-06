cmake_minimum_required(VERSION 3.1.0)
project(json VERSION 1.0)

file(GLOB src_files "simdjson.cpp")

add_library(json STATIC ${src_files})

target_include_directories(json
	PUBLIC
	$<INSTALL_INTERFACE:include>    
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)