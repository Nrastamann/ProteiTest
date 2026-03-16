cmake_minimum_required(VERSION 3.25)

project(ProteiPractice
        LANGUAGES CXX
        DESCRIPTION "Homework for PROTEI practice"
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

#sanitizers
option(SANITIZERS "Build with sanitizers" OFF)
option(BUILD_TESTS "Build tests" ON)
option(TEST_COVERAGE "Build with coverage" OFF)
option(cppcheck "Run CppCheck static code analysis" OFF)

add_executable(proteip src/main.cpp)

set(SRCS
        src/logger.cpp  
        src/menu.cpp            
        src/parsing.cpp   
        src/custom_types.cpp
        src/display.cpp      
        src/menu_functions.cpp  
        src/settings.cpp
)

target_compile_options(proteip PRIVATE
        -Wall -Wextra -pedantic-errors -Wsign-conversion -Wconversion -Werror -Weffc++ -Winline)


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

if(SANITIZERS)
        set(SANITIZER_FLAGS -g -fsanitize=address -Wno-error -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=leak -fsanitize=pointer-compare -fsanitize=pointer-subtract)
 
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-O1)
  endif()
  
  add_link_options(${SANITIZER_FLAGS})
endif()

add_compile_options(${SANITIZER_FLAGS})

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

if(TEST_COVERAGE)
        set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS}--coverage")
        set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS}--coverage")

        add_link_options(${CMAKE_C_FLAGS_COVERAGE})
        add_link_options(${CMAKE_CXX_FLAGS_COVERAGE})
        target_compile_options(proteip.test PRIVATE
                -fprofile-arcs -ftest-coverage -fprofile-instr-generate -fcoverage-mapping)
endif()



target_link_libraries(proteip.test PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(proteip.test)
