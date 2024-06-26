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
    float inc_w = 256.0;
    float inc_h = 256.0;
    float inc_d = 256.0;
    float this_w = 256.0;
    float this_h = 256.0;
    float this_d = 256.0;

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    StackFrame stack[9];
    stack[0].low = first_low;
    stack[0].curr_node_id = node_id;
    stack[0].left_off = 0;
    int level = 0;
    while (level >= 0 && level < 8) {
        StackFrame stack_frame = stack[level];
        vec3 low = stack_frame.low;
        uint curr_node_id = stack_frame.curr_node_id;
        uint curr_node_masks = voxel_buffers[volume_id].voxels[curr_node_id];
        uint left_off = stack_frame.left_off;
        vec3 diff = vec3(sub_w, sub_h, sub_d) * float(1 << (8 - level));
        for (uint idx = left_off; idx < 8; ++idx) {
            uint child = direction_kind ^ idx;
            uint valid = (curr_node_masks >> (7 - child)) & 1;
            if (bool(valid)) {
                vec3 sub_low = low + subpositions(child) * diff * 0.5;
                vec3 sub_high = sub_low + diff * 0.5;
                aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
                if (hit.front_t != -FAR_AWAY) {
                    uint leaf = (curr_node_masks >> (15 - child)) & 1;
                    uint num_valid = bitCount((curr_node_masks & 0xFF) >> (8 - child));
                    uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id + 1 + num_valid];
                    if (bool(leaf)) {
                        if (intersect_format_2(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                            return true;
                        }
                    } else {
                        stack[level].low = low;
                        stack[level].curr_node_id = curr_node_id;
                        stack[level].left_off = idx + 1;
                        ++level;
                        stack[level].low = sub_low;
                        stack[level].curr_node_id = child_node_id;
                        stack[level].left_off = 0;
                        ++level;
                        break;
                    }
                }
            }
        }
        --level;
    }
    return false;
}

bool intersect_format_0(uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    float sub_w = 256.0;
    float sub_h = 256.0;
    float sub_d = 256.0;
    float inc_w = 2048.0;
    float inc_h = 2048.0;
    float inc_d = 2048.0;
    float this_w = 8.0;
    float this_h = 8.0;
    float this_d = 8.0;

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    StackFrame stack[4];
    stack[0].low = first_low;
    stack[0].curr_node_id = node_id;
    stack[0].left_off = 0;
    int level = 0;
    while (level >= 0 && level < 3) {
        StackFrame stack_frame = stack[level];
        vec3 low = stack_frame.low;
        uint curr_node_id = stack_frame.curr_node_id;
        uint curr_node_child = voxel_buffers[volume_id].voxels[curr_node_id];
        uint curr_node_masks = voxel_buffers[volume_id].voxels[curr_node_id + 1];
        uint left_off = stack_frame.left_off;
        vec3 diff = vec3(sub_w, sub_h, sub_d) * float(1 << (3 - level));
        for (uint idx = left_off; idx < 8; ++idx) {
            uint child = direction_kind ^ idx;
            uint valid = (curr_node_masks >> (7 - child)) & 1;
            if (bool(valid)) {
                vec3 sub_low = low + subpositions(child) * diff * 0.5;
                vec3 sub_high = sub_low + diff * 0.5;
                aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
                if (hit.front_t != -FAR_AWAY) {
                    uint leaf = (curr_node_masks >> (15 - child)) & 1;
                    uint num_valid = bitCount((curr_node_masks & 0xFF) >> (8 - child));
                    uint child_node_id = curr_node_child + num_valid * 2;
                    if (bool(leaf)) {
                        if (intersect_format_1(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                            return true;
                        }
                    } else {
                        stack[level].low = low;
                        stack[level].curr_node_id = curr_node_id;
                        stack[level].left_off = idx + 1;
                        ++level;
                        stack[level].low = sub_low;
                        stack[level].curr_node_id = child_node_id;
                        stack[level].left_off = 0;
                        ++level;
                        break;
                    }
                }
            }
        }
        --level;
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
