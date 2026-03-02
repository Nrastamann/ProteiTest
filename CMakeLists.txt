cmake_minimum_required(VERSION 3.25)

project(ProteiPractice)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

option(SANITIZERS "Build with sanitizers" OFF)

if(SANITIZERS)
  set(SANITIZER_FLAGS -g -fsanitize=address -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=leak -fsanitize=pointer-compare -fsanitize=pointer-subtract)
  add_compile_options(${SANITIZER_FLAGS})
  
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-O1)
  endif()
  
  add_link_options(${SANITIZER_FLAGS})
endif()

add_compile_options(-Wall -Wextra -Werror)

SET(LOG_VERBOSITY "Trace" CACHE STRING "Verbosity log")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h" @ONLY)

add_executable(task1)
target_include_directories(task1 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.cpp)
target_sources(task1 PRIVATE ${SRCS})


set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_INCLUDE_LIBRARIES system asio)
set(BOOST_ENABLE_CMAKE ON)

find_package(Boost)
find_package(GTest)


if (Boost_FOUND)
        target_link_libraries(task1 PRIVATE ${Boost_LIBRARIES})
endif()
