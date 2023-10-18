cmake_minimum_required(VERSION 3.11.0)
project(ArduPlot VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(source_dir "${PROJECT_SOURCE_DIR}/src")
file(GLOB_RECURSE src_files "${source_dir}/*.cpp")

message(STATUS "Source Dir: ${source_dir}")
message(STATUS "Files: ${src_files}")

add_executable(${PROJECT_NAME} ${src_files})

add_subdirectory(libs/glfw)
add_subdirectory(libs/glad)
add_subdirectory(libs/SerialPort)
add_subdirectory(libs/json)
add_subdirectory(libs/imgui)
add_subdirectory(libs/implot)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message("------------------------------------")
message(STATUS "Downloading Tracy client library")
message("------------------------------------")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	message(STATUS "Release build detected, disabling TRACY_ENABLE")
	set(TRACY_ENABLE "OFF" CACHE INTERNAL "tracy_enable")
else()
	message(STATUS "Debug build detected, enabling TRACY_ENABLE")
	set(TRACY_ENABLE "ON" CACHE INTERNAL "tracy_enable")
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

set(GLFW_BUILD_DOCS "OFF" CACHE INTERNAL "disable_glfw_docs_build")

include(FetchContent)

FetchContent_Declare (
	tracy
	GIT_REPOSITORY https://github.com/wolfpld/tracy.git
	GIT_TAG 897aec5b062664d2485f4f9a213715d2e527e0ca
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(tracy)

target_include_directories(${PROJECT_NAME} PUBLIC
	./
	include
	libs/glfw/include
	libs/glad/include
	libs/SerialPort/include
	libs/imgui/include
	libs/implot/
	libs/json/
	libs/
	${tracy_SOURCE_DIR}/public/
)

target_link_libraries(${PROJECT_NAME} imgui implot glfw glad SerialPort json TracyClient)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	# If the debug configuration pass the DEBUG define to the compiler 
	$<$<CONFIG:Debug>:DEBUG>
)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/imgui.ini ${PROJECT_BINARY_DIR}/imgui.ini
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/config.default.ini ${PROJECT_BINARY_DIR}/config.default.ini
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/resources/ ${PROJECT_BINARY_DIR}/resources/
)
