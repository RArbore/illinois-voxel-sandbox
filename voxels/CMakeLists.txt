cmake_minimum_required(VERSION 3.0)

add_library(voxels VoxelChunk.cpp VoxelManager.cpp)

target_link_libraries(voxels PRIVATE glm)
