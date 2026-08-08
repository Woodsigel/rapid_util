cmake_minimum_required(VERSION 3.16)

project(rapidjs_util)

option(rapidjsUtil_BUILD_TESTS "Build tests" ON)


set(CMAKE_CXX_STANDARD 17)            # Require C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)   
set(CMAKE_CXX_EXTENSIONS OFF)        

add_executable(rapidjsUtilMarshal marshal_example.cpp)
add_executable(rapidjsUtilUnmarshal unmarshal_example.cpp)
add_executable(rapidjsUtilAlias alias_example.cpp)
 
# Download and build rapidjsON if not found
find_package(rapidjsON QUIET)
if(NOT rapidjsON_FOUND)
	include(ExternalProject)
	ExternalProject_Add(
		rapidjson
		GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
		GIT_TAG v1.1.0  
		CONFIGURE_COMMAND ""  # No configure needed for header-only
		BUILD_COMMAND ""      # No build needed for header-only  
		INSTALL_COMMAND ""    # No install needed for header-only

	)
	
	ExternalProject_Get_property(rapidjson SOURCE_DIR)
	set(rapidjsON_INCLUDE_DIRS "${SOURCE_DIR}/include")
	
	add_dependencies(rapidjsUtilMarshal rapidjson)
	add_dependencies(rapidjsUtilUnmarshal rapidjson)
endif()


set(rapidjsUtil_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" ${rapidjsON_INCLUDE_DIRS})

target_include_directories(rapidjsUtilMarshal PRIVATE ${rapidjsUtil_INCLUDE_DIR})
target_include_directories(rapidjsUtilUnmarshal PRIVATE ${rapidjsUtil_INCLUDE_DIR})
target_include_directories(rapidjsUtilAlias PRIVATE ${rapidjsUtil_INCLUDE_DIR})

if(rapidjsUtil_BUILD_TESTS)
	message(STATUS "Building with tests")
	enable_testing()
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()