cmake_minimum_required(VERSION 3.0)

add_library(
	voxels 
	Voxel.cpp 
	VoxelChunkGeneration.cpp 
	Conversion.cpp 
	Voxelize.cpp 
	raw_24_24_24_raw_16_16_16_construct.cpp
	df_24_24_24_8_raw_16_16_16_construct.cpp
	svo_9_construct.cpp
	svdag_9_construct.cpp
	df_12_12_12_12_svo_5_construct.cpp
	df_12_12_12_12_svdag_5_construct.cpp
	)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(voxels PRIVATE graphics)
