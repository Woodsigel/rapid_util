# Download and build GTest if not found
find_package(GTest QUIET)
if(NOT TARGET GTest::gmock)
	include(FetchContent)
	FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
	)
	
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
endif()

set(TESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(TESTS_SRCS ${TESTS_SOURCE_DIR}/rapidjs_util_test.cpp
               ${TESTS_SOURCE_DIR}/type_traits_test.cpp
			   ${TESTS_SOURCE_DIR}/marshal_test.cpp
			   ${TESTS_SOURCE_DIR}/unmarshal_test.cpp
               ${TESTS_SOURCE_DIR}/value_test.cpp)
			  
add_executable(rapidjsUtilTest ${TESTS_SRCS})
target_include_directories(rapidjsUtilTest PRIVATE ${RapidJSON_INCLUDE_DIR} ${RapidJSON_UTIL_INCLUDE})
target_link_libraries(rapidjsUtilTest GTest::gmock)

add_dependencies(rapidjsUtilTest rapidjson)
     
if(MSVC)
	target_compile_options(rapidjsUtilTest PRIVATE $<$<CONFIG:Debug>:/MD>)
endif()