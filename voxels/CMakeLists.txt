cmake_minimum_required(VERSION 3.0)

add_library(voxels Voxel.cpp VoxelChunkGeneration.cpp Conversion.cpp Voxelize.cpp)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(voxels PRIVATE graphics)
