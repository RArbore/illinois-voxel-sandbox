cmake_minimum_required(VERSION 3.0)
project(illinois-voxel-sandbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(external)
add_subdirectory(graphics)
add_subdirectory(utils)
add_subdirectory(drivers)
add_subdirectory(shaders)
