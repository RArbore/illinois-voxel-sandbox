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
    float inc_w = 8.0;
    float inc_h = 8.0;
    float inc_d = 8.0;
    float this_w = 8.0;
    float this_h = 8.0;
    float this_d = 8.0;

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    vec3 first_high = lower + vec3(inc_w, inc_h, inc_d);
    vec4 stack[4];
    stack[0] = vec4(first_low, uintBitsToFloat(node_id & 0x1FFFFFFF));
    int level = 0;
    while (level >= 0 && level < 4) {
        vec4 stack_frame = stack[level];
        vec3 low = stack_frame.xyz;
        uint curr_node_id = floatBitsToUint(stack_frame.w) & 0x1FFFFFFF;
        uint curr_node_child = voxel_buffers[volume_id].voxels[curr_node_id];
        uint curr_node_masks = voxel_buffers[volume_id].voxels[curr_node_id + 1];
        uint left_off = floatBitsToUint(stack_frame.w) >> 29;
        float diff = float(1 << 4) * pow(0.5, level + 1);
        for (uint idx = left_off; idx < 8; ++idx) {
            uint child = direction_kind ^ idx;
            uint valid = (curr_node_masks >> (7 - child)) & 1;
            if (bool(valid)) {
                vec3 sub_low = low + subpositions(child) * diff;
                vec3 sub_high = sub_low + diff;
                aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
                if (hit.front_t != -FAR_AWAY) {
                    uint leaf = (curr_node_masks >> (15 - child)) & 1;
                    uint num_valid = bitCount((curr_node_masks & 0xFF) >> (8 - child));
                    uint child_node_id = curr_node_child + num_valid;
                    if (bool(leaf)) {
                        if (intersect_format_1(volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                            return true;
                        } else if (idx != 7) {
                            stack[level] = vec4(low, uintBitsToFloat(curr_node_id | ((idx + 1) << 29)));
                            ++level;
                        }
                    } else if (idx == 7) {
                        stack[level] = vec4(low, uintBitsToFloat(child_node_id));
                        ++level;
                    } else {
                        stack[level] = vec4(low, uintBitsToFloat(curr_node_id | ((idx + 1) << 29)));
                        ++level;
                        stack[level] = vec4(low, uintBitsToFloat(child_node_id));
                        ++level;
                    }
                    break;
                }
            }
        }
        --level;
    }
}

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    uint root_id = voxel_buffers[volume_id].voxels[0];

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(8.0, 8.0, 8.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
        intersect_format_0(volume_id, root_id, obj_ray_pos, obj_ray_dir, vec3(0.0), r.front_t, r.k);
    }
}