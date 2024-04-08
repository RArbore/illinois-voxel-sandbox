cmake_minimum_required(VERSION 3.0)

add_custom_target(shaders)

set(GLSL_SHADER_FILES
	rgen.glsl
	rmiss.glsl
	Raw_Color_rchit.glsl
	Raw_Color_rint.glsl
	SVO_Color_rchit.glsl
	SVO_Color_rint.glsl
	SVDAG_Color_rchit.glsl
	SVDAG_Color_rint.glsl
	Unloaded_rchit.glsl
	Unloaded_rint.glsl
	tonemap_comp.glsl
	Emissive_rchit.glsl
	Download_rchit.glsl
	DF_Color_rchit.glsl
	DF_Color_rint.glsl
	Generic_rchit.glsl

	df_16_16_16_6_df_8_8_8_6_svdag_4_intersect.glsl
	df_16_16_16_6_raw_8_8_8_svdag_4_intersect.glsl
	raw_16_16_16_raw_8_8_8_svdag_4_intersect.glsl
	raw_16_16_16_svo_3_svdag_4_intersect.glsl
	raw_16_16_16_raw_16_16_16_raw_8_8_8_intersect.glsl
	df_16_16_16_6_svo_7_intersect.glsl
	df_16_16_16_6_svdag_7_intersect.glsl
	df_64_64_64_6_svo_5_intersect.glsl
	df_64_64_64_6_svdag_5_intersect.glsl
	raw_16_16_16_svo_7_intersect.glsl
	raw_16_16_16_svdag_7_intersect.glsl
	raw_64_64_64_svo_5_intersect.glsl
	raw_64_64_64_svdag_5_intersect.glsl
	raw_8_8_8_svdag_8_intersect.glsl
	raw_256_256_256_svdag_3_intersect.glsl
	svo_7_svdag_4_intersect.glsl
	svo_5_svdag_6_intersect.glsl
	svo_3_svdag_8_intersect.glsl
	svo_11_intersect.glsl
	svdag_11_intersect.glsl
	df_8_8_8_6_df_8_8_8_6_svdag_3_intersect.glsl
	df_8_8_8_6_raw_8_8_8_svdag_3_intersect.glsl
	raw_8_8_8_raw_8_8_8_svdag_3_intersect.glsl
	raw_8_8_8_raw_8_8_8_raw_8_8_8_intersect.glsl
	raw_16_16_16_raw_2_2_2_raw_16_16_16_intersect.glsl
	df_16_16_16_6_svo_5_intersect.glsl
	df_16_16_16_6_svdag_5_intersect.glsl
	raw_16_16_16_svo_5_intersect.glsl
	raw_16_16_16_svdag_5_intersect.glsl
	raw_4_4_4_svdag_7_intersect.glsl
	raw_128_128_128_svdag_2_intersect.glsl
	svo_5_raw_16_16_16_intersect.glsl
	svdag_5_raw_16_16_16_intersect.glsl
	df_32_32_32_6_df_16_16_16_6_intersect.glsl
	df_32_32_32_6_raw_16_16_16_intersect.glsl
	raw_32_32_32_raw_16_16_16_intersect.glsl
	raw_512_512_512_intersect.glsl
	df_512_512_512_6_intersect.glsl
	svo_9_intersect.glsl
	svdag_9_intersect.glsl
	)

foreach(FILE ${GLSL_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  add_custom_command(TARGET shaders
    COMMAND glslc --target-spv=spv1.5 --target-env=vulkan1.3 -O -g -o ${CMAKE_BINARY_DIR}/${FILE_WE}.spv ${FILE}
    MAIN_DEPENDENCY ${FILE}
    COMMENT "GLSL ${FILE}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
endforeach(FILE)
