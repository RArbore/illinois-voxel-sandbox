#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

void main() {
    uint32_t model_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    ivec3 volume_size = ivec3(chunk_dimensions[3 * model_id], chunk_dimensions[3 * model_id + 1], chunk_dimensions[3 * model_id + 2]);

    aabb_intersect_result r = hit_aabb(vec3(0.0), volume_size, obj_ray_pos, obj_ray_dir);

    if (r.front_t != -FAR_AWAY) {
	int crb_idx = int(model_id % MAX_NUM_CHUNKS_LOADED_PER_FRAME);
	int num_rays = 2 * crb_idx;
	int crb_model = 2 * crb_idx + 1;
	atomicExchange(chunk_request_buffer[crb_model], model_id);
	atomicAdd(chunk_request_buffer[num_rays], 1);
    }
}
