cmake_minimum_required(VERSION 3.25)

project(ProteiPractice
        LANGUAGES CXX
        DESCRIPTION "Homework for PROTEI practice"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_FLAGS_COVERAGE --coverage)

#log verbosity and compile options
SET(LOG_VERBOSITY "Trace" CACHE STRING "Verbosity log")
#sanitizers
option(SANITIZERS "Build with sanitizers" OFF)
option(BUILD_TESTS "Build tests" ON)

add_executable(proteip src/main.cpp)

add_compile_options(${SANITIZER_FLAGS})
 
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/config.hpp" @ONLY)

set(SRCS
        src/config.cpp   
        src/logger.cpp  
        src/menu.cpp            
        src/parsing.cpp   
        src/static_containers.cpp
        src/display.cpp      
        src/menu_functions.cpp  
        src/settings.cpp
)


target_sources(proteip 
        PRIVATE src/main.cpp ${SRCS}
        PRIVATE
                FILE_SET HEADERS
                BASE_DIRS include
)
target_compile_options(proteip PRIVATE
        -Wall -Wextra -pedantic-errors -Wsign-conversion -Wconversion -Werror -Weffc++ -Winline)

if(SANITIZERS)
        set(SANITIZER_FLAGS -g -fsanitize=address -Wno-error -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=leak -fsanitize=pointer-compare -fsanitize=pointer-subtract)
 
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-O1)
  endif()
  
  add_link_options(${SANITIZER_FLAGS})
endif()




include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG fa8438ae6b70c57010177de47a9f13d7041a6328        
)

enable_testing()

add_executable(proteip.test)

FetchContent_MakeAvailable(googletest)
set_property(
        TARGET proteip.test
        APPEND
        PROPERTY COMPILE_OPTIONS -w)

target_sources(proteip.test
    PRIVATE tests/test.cpp ${SRCS}
    PRIVATE
        FILE_SET HEADERS
        BASE_DIRS
            include
            tests
)

target_link_libraries(proteip.test PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(proteip.test)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_INCLUDE_LIBRARIES system asio)
set(BOOST_ENABLE_CMAKE ON)

find_package(Boost)

if (Boost_FOUND)
        target_link_libraries(proteip PRIVATE ${Boost_LIBRARIES})
endif()
