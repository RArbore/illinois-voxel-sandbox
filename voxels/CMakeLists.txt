cmake_minimum_required(VERSION 3.0)

add_library(voxels Voxel.cpp VoxelChunkGeneration.cpp Conversion.cpp ../external/enkimi/src/enkimi.c ../external/enkimi/src/miniz.c
        AnvilLoader.cpp
        AnvilLoader.h)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(voxels PRIVATE graphics)

target_include_directories(voxels PRIVATE ../external/enkimi/src)