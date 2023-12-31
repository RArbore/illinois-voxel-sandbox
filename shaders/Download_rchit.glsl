#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload hit;

void main() {
    uint chunk_id = gl_InstanceCustomIndexEXT;
    hit.hit = false;
    hit.color = vec4(0.0, 0.0, 0.0, 1.0);
    atomicAdd(chunk_download_buffer[chunk_id], 1);
}
