cmake_minimum_required(VERSION 3.0)

add_library(voxels Voxel.cpp)

target_link_libraries(voxels PRIVATE vector3d)