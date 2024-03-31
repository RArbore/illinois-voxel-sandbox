cmake_minimum_required(VERSION 3.0)

add_library(
	voxels 
	Voxel.cpp 
	VoxelChunkGeneration.cpp 
	Conversion.cpp 
	Voxelize.cpp 

	df_16_16_16_6_svo_8_construct.cpp
	df_16_16_16_6_svdag_8_construct.cpp
	df_64_64_64_6_svo_6_construct.cpp
	df_64_64_64_6_svdag_6_construct.cpp
	svo_8_raw_16_16_16_construct.cpp
	svdag_8_raw_16_16_16_construct.cpp
	df_16_16_16_6_raw_16_16_16_svdag_4_construct.cpp
	svo_12_construct.cpp
	svdag_12_construct.cpp
	df_16_16_16_6_svo_5_construct.cpp
	df_16_16_16_6_svdag_5_construct.cpp
	svo_5_raw_16_16_16_construct.cpp
	svdag_5_raw_16_16_16_construct.cpp
	raw_512_512_512_construct.cpp
	raw_32_32_32_raw_16_16_16_construct.cpp
	raw_8_8_8_raw_8_8_8_raw_8_8_8_construct.cpp
	df_512_512_512_6_construct.cpp
	df_32_32_32_6_df_16_16_16_6_construct.cpp
	df_8_8_8_6_raw_8_8_8_svdag_3_construct.cpp
	df_8_8_8_6_df_8_8_8_6_svdag_3_construct.cpp
	svo_9_construct.cpp
	svdag_9_construct.cpp
	)

target_include_directories(voxels PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(voxels PRIVATE graphics)
