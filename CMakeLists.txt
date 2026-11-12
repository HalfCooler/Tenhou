cmake_minimum_required(VERSION 3.24)

project(Tenhou LANGUAGES CXX)

set(CMAKE_BUILD_PARALLEL_LEVEL 8)

add_subdirectory(Tenhou.Client)
add_subdirectory(Tenhou.Server)