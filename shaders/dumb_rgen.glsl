#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0, rgba8) uniform image2D output_image;

void main() {
    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), vec4(1.0, 0.0, 1.0, 1.0));
}
