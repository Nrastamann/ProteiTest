cmake_minimum_required(VERSION 3.25)

project(ProteiPractice)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -Wextra -Werror -g)
add_executable(task1)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.cpp)
target_sources(task1 PRIVATE ${SRCS})

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_INCLUDE_LIBRARIES system asio)
set(BOOST_ENABLE_CMAKE ON)

find_package(Boost)

if (Boost_FOUND)
        target_link_libraries(task1 PRIVATE ${Boost_LIBRARIES})
endif()
