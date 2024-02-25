#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

vec3 subpositions(uint child) {
    return vec3(
		bool(child % 2) ? 1.0 : 0.0,
		bool((child / 2) % 2) ? 1.0 : 0.0,
		bool((child / 4) % 2) ? 1.0 : 0.0
		);
}

bool intersect_format_2(uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    leaf_id = node_id;
    float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_pos + obj_ray_dir * o_t, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
    reportIntersectionEXT(intersect_time, hit_kind);
    return true;
}

bool intersect_format_1(uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    float sub_w = 1.0;
    float sub_h = 1.0;
    float sub_d = 1.0;
    float inc_w = 32.0;
    float inc_h = 32.0;
    float inc_d = 32.0;
    float this_w = 32.0;
    float this_h = 32.0;
    float this_d = 32.0;

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 low_0 = lower;
    uint curr_node_id_0 = node_id;
    float diff_0 = float(1 << (5 - 0));
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id_0 + child];
        if (child_node_id != 0) {
            vec3 sub_low = low_0 + subpositions(child) * diff_0 * 0.5;
            vec3 sub_high = sub_low + diff_0 * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                    if (intersect_format_2(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_1 = child_node_id;
                    vec3 low_1 = sub_low;

    float diff_1 = float(1 << (5 - 1));
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id_1 + child];
        if (child_node_id != 0) {
            vec3 sub_low = low_1 + subpositions(child) * diff_1 * 0.5;
            vec3 sub_high = sub_low + diff_1 * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                    if (intersect_format_2(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_2 = child_node_id;
                    vec3 low_2 = sub_low;

    float diff_2 = float(1 << (5 - 2));
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id_2 + child];
        if (child_node_id != 0) {
            vec3 sub_low = low_2 + subpositions(child) * diff_2 * 0.5;
            vec3 sub_high = sub_low + diff_2 * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                    if (intersect_format_2(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_3 = child_node_id;
                    vec3 low_3 = sub_low;

    float diff_3 = float(1 << (5 - 3));
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id_3 + child];
        if (child_node_id != 0) {
            vec3 sub_low = low_3 + subpositions(child) * diff_3 * 0.5;
            vec3 sub_high = sub_low + diff_3 * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                    if (intersect_format_2(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_4 = child_node_id;
                    vec3 low_4 = sub_low;

    float diff_4 = float(1 << (5 - 4));
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id_4 + child];
        if (child_node_id != 0) {
            vec3 sub_low = low_4 + subpositions(child) * diff_4 * 0.5;
            vec3 sub_high = sub_low + diff_4 * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                    if (intersect_format_2(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_5 = child_node_id;
                    vec3 low_5 = sub_low;

                }
            }
        }
    }
                }
            }
        }
    }
                }
            }
        }
    }
                }
            }
        }
    }
                }
            }
        }
    }
    return false;
}

bool intersect_format_0(uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    float sub_w = 32.0;
    float sub_h = 32.0;
    float sub_d = 32.0;
    float inc_w = 384.0;
    float inc_h = 384.0;
    float inc_d = 384.0;
    float this_w = 12.0;
    float this_h = 12.0;
    float this_d = 12.0;

    vec3 chunk_ray_pos = (obj_ray_pos - lower) / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(o_t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3(11, 11, 11)));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    uint budget = 0;
    while (all(greaterThanEqual(chunk_ray_voxel, ivec3(0))) && all(lessThan(chunk_ray_voxel, ivec3(12, 12, 12)))) {
        if (budget == 0) {
            uint32_t voxel_index = linearize_index(chunk_ray_voxel, 12, 12, 12);
            uint32_t child_id = voxel_buffers[volume_id].voxels[node_id + 2 * voxel_index];
            budget = voxel_buffers[volume_id].voxels[node_id + 2 * voxel_index + 1];
            
            if (child_id != 0) {
                aabb_intersect_result r = hit_aabb(chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, (chunk_ray_voxel + 1) * vec3(sub_w, sub_h, sub_d) + lower, obj_ray_pos, obj_ray_dir);
                if (intersect_format_1(volume_id, child_id, obj_ray_pos, obj_ray_dir, chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, r.front_t, r.k)) {
                    return true;
                }
            }
        }

        bvec3 mask = lessThanEqual(chunk_side_dist.xyz, min(chunk_side_dist.yzx, chunk_side_dist.zxy));
        chunk_side_dist += vec3(mask) * chunk_ray_delta;
        chunk_ray_voxel += ivec3(mask) * chunk_ray_step;
        --budget;
    }
    return false;
}

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    uint root_id = voxel_buffers[volume_id].voxels[0];

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(384.0, 384.0, 384.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
        intersect_format_0(volume_id, root_id, obj_ray_pos, obj_ray_dir, vec3(0.0), r.front_t, r.k);
    }
}
