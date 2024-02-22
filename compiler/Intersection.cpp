#include <sstream>

#include <utils/Assert.h>

#include "Compiler.h"

std::string generate_intersection_glsl(const std::vector<InstantiatedFormat> &format, const Optimizations &opt) {
    std::stringstream ss;
    
    auto [total_w, total_h, total_d] = calculate_bounds(format);
    
    ss << R"(#version 460
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

bool intersect_format_)" << format.size() << R"((uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
    leaf_id = node_id;
    float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_pos + obj_ray_dir * o_t, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
    reportIntersectionEXT(intersect_time, hit_kind);
    return true;
}
)";

    for (int32_t i = format.size() - 1; i >= 0; --i) {
	ss << R"(
bool intersect_format_)" << i << R"((uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, vec3 lower, float o_t, uint hit_kind) {
)";
	
	auto [sub_w, sub_h, sub_d] = calculate_bounds(format, i + 1);
	auto [inc_w, inc_h, inc_d] = calculate_bounds(format, i);
	auto this_w = inc_w / sub_w, this_h = inc_h / sub_h, this_d = inc_d / sub_d;

	ss << R"(    float sub_w = )" << sub_w << R"(.0;
    float sub_h = )" << sub_h << R"(.0;
    float sub_d = )" << sub_d << R"(.0;
    float inc_w = )" << inc_w << R"(.0;
    float inc_h = )" << inc_h << R"(.0;
    float inc_d = )" << inc_d << R"(.0;
    float this_w = )" << this_w << R"(.0;
    float this_h = )" << this_h << R"(.0;
    float this_d = )" << this_d << R"(.0;

)";

	switch (format[i].format_) {
	case Format::Raw:
	    ss << R"(    vec3 chunk_ray_pos = (obj_ray_pos - lower) / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(o_t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3()" << (this_w - 1) << R"(, )" <<  (this_h - 1) << R"(, )" << (this_d - 1) << R"()));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    while (all(greaterThanEqual(chunk_ray_voxel, ivec3(0))) && all(lessThan(chunk_ray_voxel, ivec3()" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"()))) {
        uint32_t voxel_index = linearize_index(chunk_ray_voxel, )" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"();
        uint32_t child_id = voxel_buffers[volume_id].voxels[node_id + voxel_index];

        if (child_id != 0) {
            aabb_intersect_result r = hit_aabb(chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, (chunk_ray_voxel + 1) * vec3(sub_w, sub_h, sub_d) + lower, obj_ray_pos, obj_ray_dir);
            if (intersect_format_)" << i + 1 << R"((volume_id, child_id, obj_ray_pos, obj_ray_dir, chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, r.front_t, r.k)) {
                return true;
            }
        }

        bvec3 mask = lessThanEqual(chunk_side_dist.xyz, min(chunk_side_dist.yzx, chunk_side_dist.zxy));
        chunk_side_dist += vec3(mask) * chunk_ray_delta;
        chunk_ray_voxel += ivec3(mask) * chunk_ray_step;
    }
    return false;
)";
	    break;
	case Format::DF:
	    ss << R"(    vec3 chunk_ray_pos = (obj_ray_pos - lower) / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(o_t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3()" << (this_w - 1) << R"(, )" <<  (this_h - 1) << R"(, )" << (this_d - 1) << R"()));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    uint budget = 0;
    while (all(greaterThanEqual(chunk_ray_voxel, ivec3(0))) && all(lessThan(chunk_ray_voxel, ivec3()" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"()))) {
        if (budget == 0) {
            uint32_t voxel_index = linearize_index(chunk_ray_voxel, )" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"();
            uint32_t child_id = voxel_buffers[volume_id].voxels[node_id + 2 * voxel_index];
            budget = voxel_buffers[volume_id].voxels[node_id + 2 * voxel_index + 1];
            
            if (child_id != 0) {
                aabb_intersect_result r = hit_aabb(chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, (chunk_ray_voxel + 1) * vec3(sub_w, sub_h, sub_d) + lower, obj_ray_pos, obj_ray_dir);
                if (intersect_format_)" << i + 1 << R"((volume_id, child_id, obj_ray_pos, obj_ray_dir, chunk_ray_voxel * vec3(sub_w, sub_h, sub_d) + lower, r.front_t, r.k)) {
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
)";
	    break;
	case Format::SVO:
	    ss << R"(    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    vec3 first_high = lower + vec3(inc_w, inc_h, inc_d);
    vec4 stack[)" << (format[i].parameters_[0] + 2) << R"(];
    stack[0] = vec4(first_low, uintBitsToFloat(node_id & 0x0FFFFFFF));
    int level = 0;
    while (level >= 0 && level < )" << (format[i].parameters_[0] + 1) << R"() {
        vec4 stack_frame = stack[level];
        vec3 low = stack_frame.xyz;
        uint curr_node_id = floatBitsToUint(stack_frame.w) & 0x0FFFFFFF;
        uint curr_node_child = voxel_buffers[volume_id].voxels[curr_node_id];
        uint curr_node_masks = voxel_buffers[volume_id].voxels[curr_node_id + 1];
        uint left_off = floatBitsToUint(stack_frame.w) >> 28;
        float diff = float(1 << )" << format[i].parameters_[0] << R"() * pow(0.5, level + 1);
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
                    uint child_node_id = curr_node_child + num_valid * 2;
                    if (bool(leaf)) {
                        if (intersect_format_)" << i + 1 << R"((volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                            return true;
                        }
                    } else {
                        stack[level] = vec4(low, uintBitsToFloat(curr_node_id | ((idx + 1) << 28)));
                        ++level;
                        stack[level] = vec4(sub_low, uintBitsToFloat(child_node_id));
                        ++level;
                        break;
                    }
                }
            }
        }
        --level;
    }
    return false;
)";
	    break;
	case Format::SVDAG:
	    ss << R"(    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    vec3 first_high = lower + vec3(inc_w, inc_h, inc_d);
    vec4 stack[)" << (format[i].parameters_[0] + 2) << R"(];
    stack[0] = vec4(first_low, uintBitsToFloat(node_id & 0x0FFFFFFF));
    int level = 0;
    while (level >= 0 && level < )" << (format[i].parameters_[0] + 1) << R"() {
        vec4 stack_frame = stack[level];
        vec3 low = stack_frame.xyz;
        uint curr_node_id = floatBitsToUint(stack_frame.w) & 0x0FFFFFFF;
        uint left_off = floatBitsToUint(stack_frame.w) >> 28;
        float diff = float(1 << )" << format[i].parameters_[0] << R"() * pow(0.5, level + 1);
        for (uint idx = left_off; idx < 8; ++idx) {
            uint child = direction_kind ^ idx;
            uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id + child];
            if (child_node_id != 0) {
                vec3 sub_low = low + subpositions(child) * diff;
                vec3 sub_high = sub_low + diff;
                aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
                if (hit.front_t != -FAR_AWAY) {
                    if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                        if (intersect_format_)" << i + 1 << R"((volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                            return true;
                        }
                    } else {
                        stack[level] = vec4(low, uintBitsToFloat(curr_node_id | ((idx + 1) << 28)));
                        ++level;
                        stack[level] = vec4(sub_low, uintBitsToFloat(child_node_id));
                        ++level;
                        break;
                    }
                }
            }
        }
        --level;
    }
    return false;
)";
	    break;
	default:
	    ASSERT(false, "Unhandled format.");
	}

	ss << R"(}
)";
    }
    
    ss << R"(
void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    uint root_id = voxel_buffers[volume_id].voxels[0];

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3()" << total_w << R"(.0, )" << total_h << R"(.0, )" << total_d << R"(.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
        intersect_format_0(volume_id, root_id, obj_ray_pos, obj_ray_dir, vec3(0.0), r.front_t, r.k);
    }
}
)";
    
    return ss.str();
}
