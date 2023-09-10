cmake_minimum_required(VERSION 3.0)

add_library(voxels Voxel.cpp)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
