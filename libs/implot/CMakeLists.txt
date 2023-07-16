cmake_minimum_required(VERSION 3.1.0)
project(implot VERSION 1.0 LANGUAGES C)

set(source_dir "${PROJECT_SOURCE_DIR}/")
file(GLOB src_files "${source_dir}/*.cpp")

add_library(implot STATIC ${src_files})
target_link_libraries(${PROJECT_NAME} imgui)
target_include_directories(implot
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../imgui/include>
	$<INSTALL_INTERFACE:include>    
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)