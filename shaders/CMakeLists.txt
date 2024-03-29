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

	df_16_16_16_6_svo_8_intersect.glsl
	df_16_16_16_6_svdag_8_intersect.glsl
	df_64_64_64_6_svo_6_intersect.glsl
	df_64_64_64_6_svdag_6_intersect.glsl
	svo_8_raw_16_16_16_intersect.glsl
	svdag_8_raw_16_16_16_intersect.glsl
	df_16_16_16_6_raw_16_16_16_svdag_4_intersect.glsl
	svo_12_intersect.glsl
	svdag_12_intersect.glsl
	df_16_16_16_6_svo_5_intersect.glsl
	df_16_16_16_6_svdag_5_intersect.glsl
	svo_5_raw_16_16_16_intersect.glsl
	svdag_5_raw_16_16_16_intersect.glsl
	raw_512_512_512_intersect.glsl
	raw_32_32_32_raw_16_16_16_intersect.glsl
	raw_8_8_8_raw_8_8_8_raw_8_8_8_intersect.glsl
	df_512_512_512_6_intersect.glsl
	df_32_32_32_6_df_16_16_16_6_intersect.glsl
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
