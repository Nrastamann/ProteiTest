cmake_minimum_required(VERSION 3.25)

project(ProteiPractice
        LANGUAGES CXX
        DESCRIPTION "Homework for PROTEI practice"
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
#sanitizers
option(SANITIZERS "Build with sanitizers" OFF)
option(TEST_COVERAGE "Build with coverage" OFF)
option(CPPCHECK "Run CppCheck static code analysis" OFF)

add_executable(proteip src/main.cpp)
add_executable(proteip.server src/server.cpp)
add_executable(proteip.benchmark)
add_executable(proteip.test)

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
        -Wall -Wextra -Wsign-conversion -Wconversion)

target_compile_options(proteip.server PRIVATE
        -Wall -Wextra -Wsign-conversion -Wconversion)

if(CPPCHECK)
  find_program(cppcheck_exe NAMES cppcheck REQUIRED)
  set(cppcheck_opts --enable=all --inline-suppr --quiet --suppressions-list=${PROJECT_SOURCE_DIR}/cppcheck.supp)
  set(CMAKE_C_CPPCHECK ${cppcheck_exe} --std=c23 ${cppcheck_opts})
  set(CMAKE_CXX_CPPCHECK ${cppcheck_exe} --std=c++23 ${cppcheck_opts})
endif()

if(TEST_COVERAGE)
        set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS}--coverage")
        set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS}--coverage")

        add_link_options(${CMAKE_C_FLAGS_COVERAGE})
        add_link_options(${CMAKE_CXX_FLAGS_COVERAGE})
        target_compile_options(proteip.test PRIVATE
                -fprofile-arcs -ftest-coverage -fprofile-instr-generate -fcoverage-mapping)
endif()
if(SANITIZERS)
        set(SANITIZER_FLAGS -g -fsanitize=address -Wno-error -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=leak -fsanitize=pointer-compare -fsanitize=pointer-subtract)
 
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-O1)
  endif()
  
  add_link_options(${SANITIZER_FLAGS})
endif()

add_compile_options(${SANITIZER_FLAGS})


target_sources(proteip 
        PRIVATE src/main.cpp ${SRCS}
        PRIVATE
                FILE_SET HEADERS
                BASE_DIRS include
)

target_sources(proteip.server 
        PRIVATE src/server.cpp ${SRCS}
        PRIVATE
                FILE_SET HEADERS
                BASE_DIRS include
)

target_sources(proteip.test
    PRIVATE tests/test.cpp ${SRCS}
    PRIVATE
        FILE_SET HEADERS
        BASE_DIRS
            include
            tests
)

include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG fa8438ae6b70c57010177de47a9f13d7041a6328        
)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)


FetchContent_MakeAvailable(googletest)
set_property(
        TARGET proteip.test
        APPEND
        PROPERTY COMPILE_OPTIONS -w)

include(GoogleTest)

gtest_discover_tests(proteip.test)
enable_testing()

target_link_libraries(proteip.test PRIVATE GTest::gtest_main)

FetchContent_Declare( googlebenchmark
GIT_REPOSITORY https://github.com/google/benchmark.git
GIT_TAG main )

FetchContent_MakeAvailable(googlebenchmark)
target_sources(proteip.benchmark
    PRIVATE benchmark/benchmark.cpp ${SRCS}
    PRIVATE
        FILE_SET HEADERS
        BASE_DIRS
            include
            benchmark
)

target_link_libraries(proteip.benchmark PRIVATE benchmark::benchmark)


FetchContent_MakeAvailable(json)

target_link_libraries(proteip PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(proteip.server PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(proteip.test PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(proteip.benchmark PRIVATE nlohmann_json::nlohmann_json)
