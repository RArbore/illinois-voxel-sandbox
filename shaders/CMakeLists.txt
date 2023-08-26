cmake_minimum_required(VERSION 3.0)

add_custom_target(shaders)

set(GLSL_SHADER_FILES dumb_rgen.glsl)

foreach(FILE ${GLSL_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  add_custom_command(TARGET shaders
    COMMAND glslc --target-spv=spv1.5 --target-env=vulkan1.2 -o ${CMAKE_BINARY_DIR}/${FILE_WE}.spv ${FILE}
    MAIN_DEPENDENCY ${FILE}
    COMMENT "GLSL ${FILE}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
endforeach(FILE)