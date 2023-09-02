cmake_minimum_required(VERSION 3.0)

set(voxel_chunks
        VoxelChunk.cpp
        RawVoxelChunk.cpp
        BrickmapVoxelChunk.cpp
)

add_library(voxels ${voxel_chunks} VoxelManager.cpp)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
