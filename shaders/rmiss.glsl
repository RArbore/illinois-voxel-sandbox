#version 460
#pragma shader_stage(miss)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.hit = false;
    payload.color = vec4(0.7, 0.81, 0.92, 3.0);
    payload.emissive = true;
}
