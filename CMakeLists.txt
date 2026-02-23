cmake_minimum_required(VERSION 3.25)

project(ProteiPractice)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall -Wextra -Werror)
add_executable(task1)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.cpp)
target_sources(task1 PRIVATE ${SRCS})
