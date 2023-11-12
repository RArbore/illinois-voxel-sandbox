#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

void main() {
    uint32_t model_id = gl_InstanceCustomIndexEXT;
    int crb_idx = int(model_id % MAX_NUM_CHUNKS_LOADED_PER_FRAME);
    int num_rays = 2 * crb_idx;
    int crb_model = 2 * crb_idx + 1;
    atomicExchange(chunk_request_buffer[crb_model], model_id);
    atomicAdd(chunk_request_buffer[num_rays], 1);
}
