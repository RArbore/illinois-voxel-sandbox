cmake_minimum_required(VERSION 3.0)

set(voxel_chunks
  VoxelChunk.cpp
  RawVoxelChunk.cpp
)

add_library(voxels ${voxel_chunks} VoxelManager.cpp)

target_link_libraries(voxels PRIVATE glm)
