#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadInEXT vec4 hit;

void main() {
    hit = vec4(1.0, 0.0, 1.0, 1.0);
}
