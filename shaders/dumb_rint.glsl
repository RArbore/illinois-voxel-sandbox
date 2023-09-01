#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 1, binding = 1, rgba8) uniform readonly image3D volume;

void main() {
    reportIntersectionEXT(1.0, imageSize(volume).x);
}
