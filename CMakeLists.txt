cmake_minimum_required(VERSION 3.0)
project(AllocineBackend)

set(CMAKE_CXX_STANDARD 11)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_FILES main.cpp movie.hpp levenshtein.hpp)
add_executable(AllocineBackend ${SOURCE_FILES})
target_link_libraries (AllocineBackend pthread uWS ssl crypto z uv)
