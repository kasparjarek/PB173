cmake_minimum_required(VERSION 2.8)
project(internet_of_tanks)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Wextra -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/)
