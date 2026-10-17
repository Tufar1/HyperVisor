cmake_minimum_required(VERSION 3.10.0)
project(hyperum VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(hyperum main.cpp)
target_link_libraries(hyperum PRIVATE)
