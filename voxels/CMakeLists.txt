cmake_minimum_required(VERSION 3.0)

add_library(
	voxels 
	Voxel.cpp 
	VoxelChunkGeneration.cpp 
	Conversion.cpp 
	Voxelize.cpp 

	df_64_64_64_6_raw_64_64_64_construct.cpp
	df_64_64_64_6_svdag_6_construct.cpp
	df_64_64_64_6_svo_6_construct.cpp
	raw_16_16_16_raw_16_16_16_raw_16_16_16_construct.cpp
	svdag_6_df_64_64_64_6_construct.cpp
	svdag_12_construct.cpp
	svo_6_df_64_64_64_6_construct.cpp
	svo_12_construct.cpp
	)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(voxels PRIVATE graphics)
