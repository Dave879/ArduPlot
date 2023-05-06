cmake_minimum_required(VERSION 3.1.0)
project(ArduPlot VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(source_dir "${PROJECT_SOURCE_DIR}/src")
file(GLOB_RECURSE src_files "${source_dir}/*.cpp")

message(STATUS "Source Dir: ${source_dir}")
message(STATUS "Files: ${src_files}")

add_executable(${PROJECT_NAME} ${src_files})

include(FetchContent) 
FetchContent_Declare(mahi-gui GIT_REPOSITORY https://github.com/mahilab/mahi-gui.git) 
FetchContent_MakeAvailable(mahi-gui)

add_subdirectory(libs/SerialPort)
add_subdirectory(libs/json)

target_include_directories(${PROJECT_NAME} PUBLIC
	./
	include
	libs/SerialPort/include
	libs/json/
)

target_link_libraries(${PROJECT_NAME} mahi::gui SerialPort json)

