#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

struct StackFrame {
    vec3 low;
    uint curr_node_id;
    uint left_off;
};

vec3 subpositions(uint child) {
    return vec3(
		bool(child % 2) ? 1.0 : 0.0,
		bool((child / 2) % 2) ? 1.0 : 0.0,
		bool((child / 4) % 2) ? 1.0 : 0.0
		);
}

uint32_t lookup_utility_restart_svo(uint volume_id, uint node_id, uint depth, ivec3 voxel) {
    ivec3 low = ivec3(0);
    uint diff = 1 << depth;
    while (true) {
        uint child_first_id = voxel_buffers[volume_id].voxels[node_id];
        uint child_masks = voxel_buffers[volume_id].voxels[node_id + 1];

        uvec3 rel_pos = voxel - low;
        bvec3 child_pos = greaterThanEqual(rel_pos, uvec3(diff / 2));
        uint child_idx = uint(child_pos.x) + 2 * uint(child_pos.y) + 4 * uint(child_pos.z);
        uint valid = (child_masks >> (7 - child_idx)) & 1;
        if (bool(valid)) {
            uint leaf = (child_masks >> (15 - child_idx)) & 1;
            uint num_valid = bitCount((child_masks & 0xFF) >> (8 - child_idx));
            uint child_node_id = child_first_id + num_valid * 2;
            if (bool(leaf)) {
                return voxel_buffers[volume_id].voxels[child_node_id];
            }
	    node_id = child_node_id;
        } else {
            return 0;
        }
        low += ivec3(uvec3(child_pos) * diff / 2);
        diff /= 2;
    }
}

uint32_t lookup_utility_restart_svdag(uint volume_id, uint node_id, uint depth, ivec3 voxel) {
    ivec3 low = ivec3(0);
    uint diff = 1 << depth;
    while (true) {
        uint child_masks = voxel_buffers[volume_id].voxels[node_id];

        uvec3 rel_pos = voxel - low;
        bvec3 child_pos = greaterThanEqual(rel_pos, uvec3(diff / 2));
        uint child_idx = uint(child_pos.x) + 2 * uint(child_pos.y) + 4 * uint(child_pos.z);
        uint valid = (child_masks >> (7 - child_idx)) & 1;
        if (bool(valid)) {
            uint leaf = (child_masks >> (15 - child_idx)) & 1;
            uint num_valid = bitCount((child_masks & 0xFF) >> (8 - child_idx));
            uint child_node_id = voxel_buffers[volume_id].voxels[node_id + 1 + num_valid];
            if (bool(leaf)) {
                return voxel_buffers[volume_id].voxels[child_node_id];
            }
	    node_id = child_node_id;
        } else {
            return 0;
        }
        low += ivec3(uvec3(child_pos) * diff / 2);
        diff /= 2;
    }
}

bool intersect_format_1(uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    leaf_id = node_id;
    float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_pos + obj_ray_dir * o_t, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
    reportIntersectionEXT(intersect_time, hit_kind);
    return true;
}

bool intersect_format_0(uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    float sub_w = 1.0;
    float sub_h = 1.0;
    float sub_d = 1.0;
    float inc_w = 2048.0;
    float inc_h = 2048.0;
    float inc_d = 2048.0;
    float this_w = 2048.0;
    float this_h = 2048.0;
    float this_d = 2048.0;

    vec3 chunk_ray_pos = (obj_ray_pos - lower) / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(o_t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3(2047, 2047, 2047)));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    while (all(greaterThanEqual(chunk_ray_voxel, ivec3(0))) && all(lessThan(chunk_ray_voxel, ivec3(2048, 2048, 2048)))) {
        uint32_t child_id = lookup_utility_restart_svo(volume_id, node_id, 11, chunk_ray_voxel);
        
        if (child_id != 0) {
            aabb_intersect_result r = hit_aabb(chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, (chunk_ray_voxel + 1) * vec3(sub_w, sub_h, sub_d) + lower, obj_ray_pos, obj_ray_dir);
            if (intersect_format_1(volume_id, child_id, obj_ray_pos, obj_ray_dir, chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, r.front_t, r.k)) {
                return true;
            }
        }

        bvec3 mask = lessThanEqual(chunk_side_dist.xyz, min(chunk_side_dist.yzx, chunk_side_dist.zxy));
        chunk_side_dist += vec3(mask) * chunk_ray_delta;
        chunk_ray_voxel += ivec3(mask) * chunk_ray_step;
    }
    return false;
}

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    uint root_id = voxel_buffers[volume_id].voxels[0];

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(2048.0, 2048.0, 2048.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
        intersect_format_0(volume_id, root_id, obj_ray_pos, obj_ray_dir, vec3(0.0), r.front_t, r.k);
    }
}
