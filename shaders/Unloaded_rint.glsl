#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

void main() {
    uint64_t volume_id = gl_InstanceCustomIndexEXT;
    int crb_idx = int(volume_id % MAX_NUM_CHUNKS_LOADED_PER_FRAME);
    atomicExchange(chunk_request_buffer[crb_idx], volume_id);
}