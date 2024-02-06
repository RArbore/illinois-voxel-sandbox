#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    ivec3 volume_size = ivec3(raw_buffers[volume_id].voxel_width, raw_buffers[volume_id].voxel_height, raw_buffers[volume_id].voxel_depth);
    aabb_intersect_result r = hit_aabb(vec3(0.0), volume_size, obj_ray_pos, obj_ray_dir);

    if (r.front_t != -FAR_AWAY) {
	vec3 obj_ray_intersect_point = obj_ray_pos + obj_ray_dir * max(r.front_t, 0.0);
	ivec3 obj_ray_voxel = ivec3(min(obj_ray_intersect_point, volume_size - 1));
	ivec3 obj_ray_step = ivec3(sign(obj_ray_dir));
	vec3 obj_ray_delta = abs(vec3(length(obj_ray_dir)) / obj_ray_dir);
	vec3 obj_side_dist = (sign(obj_ray_dir) * (vec3(obj_ray_voxel) - obj_ray_intersect_point) + (sign(obj_ray_dir) * 0.5) + 0.5) * obj_ray_delta;

	uint steps = 0;
	uint max_steps = uint(volume_size.x) + uint(volume_size.y) + uint(volume_size.z);
	while (steps < max_steps && all(greaterThanEqual(obj_ray_voxel, ivec3(0))) && all(lessThan(obj_ray_voxel, volume_size))) {
		uint32_t voxel_index = linearize_index(obj_ray_voxel, volume_size.x, volume_size.y, volume_size.z);
	    float palette = float(raw_buffers[volume_id].voxel_colors[voxel_index].alpha_);
	    
	    if (palette != 0.0) {
		aabb_intersect_result r = hit_aabb(obj_ray_voxel, obj_ray_voxel + 1, obj_ray_pos, obj_ray_dir);
		float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_pos + obj_ray_dir * r.front_t, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
		reportIntersectionEXT(intersect_time, r.k);
		return;
	    }

	    bvec3 mask = lessThanEqual(obj_side_dist.xyz, min(obj_side_dist.yzx, obj_side_dist.zxy));
	    
	    obj_side_dist += vec3(mask) * obj_ray_delta;
	    obj_ray_voxel += ivec3(mask) * obj_ray_step;
	    ++steps;
	}
    }
}
