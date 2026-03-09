cmake_minimum_required(VERSION 3.25)

project(ProteiPractice
        LANGUAGES CXX
        DESCRIPTION "Homework for PROTEI practice"
)
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS}--coverage")
set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS}--coverage")

add_link_options(${CMAKE_C_FLAGS_COVERAGE})
add_link_options(${CMAKE_CXX_FLAGS_COVERAGE})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

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

option(cppcheck "Run CppCheck static code analysis" ON)

if(cppcheck)
  find_program(cppcheck_exe NAMES cppcheck REQUIRED)
  set(cppcheck_opts --enable=all --inline-suppr --quiet --suppressions-list=${PROJECT_SOURCE_DIR}/cppcheck.supp)
  set(CMAKE_C_CPPCHECK ${cppcheck_exe} --std=c23 ${cppcheck_opts})
  set(CMAKE_CXX_CPPCHECK ${cppcheck_exe} --std=c++23 ${cppcheck_opts})
endif()

target_sources(proteip 
        PRIVATE src/main.cpp ${SRCS}
        PRIVATE
                FILE_SET HEADERS
                BASE_DIRS include
)
target_compile_options(proteip PRIVATE
        -Wall -Wextra -pedantic-errors -Wsign-conversion -Wconversion -Werror -Weffc++ -Winline -fprofile-arcs -ftest-coverage -fprofile-instr-generate -fcoverage-mapping)

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
