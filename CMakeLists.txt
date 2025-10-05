cmake_minimum_required(VERSION 3.16)

project(rapid_util)

option(RAPIDUTIL_BUILD_TESTS "Build tests" ON)


set(CMAKE_CXX_STANDARD 17)            # Require C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)   
set(CMAKE_CXX_EXTENSIONS OFF)        

add_executable(rapidUtilMarshal marshal_example.cpp)
add_executable(rapidUtilUnmarshal unmarshal_example.cpp)

# Download and build RapidJSON if not found
find_package(RapidJSON QUIET)
if(NOT RapidJSON_FOUND)
	include(ExternalProject)
	ExternalProject_Add(
		rapidjson
		GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
		GIT_TAG v1.1.0  # Specific tag
		CONFIGURE_COMMAND ""  # No configure needed for header-only
		BUILD_COMMAND ""      # No build needed for header-only  
		INSTALL_COMMAND ""    # No install needed for header-only

	)
	
	ExternalProject_Get_property(rapidjson SOURCE_DIR)
	set(RapidJSON_INCLUDE_DIRS "${SOURCE_DIR}/include")
	
	add_dependencies(rapidUtilMarshal rapidjson)
	add_dependencies(rapidUtilUnmarshal rapidjson)
endif()


set(RAPIDUTIL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" ${RapidJSON_INCLUDE_DIRS})

target_include_directories(rapidUtilMarshal PRIVATE ${RAPIDUTIL_INCLUDE_DIR})
target_include_directories(rapidUtilUnmarshal PRIVATE ${RAPIDUTIL_INCLUDE_DIR})

if(RAPIDUTIL_BUILD_TESTS)
	message(STATUS "Building with tests")
	enable_testing()
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()