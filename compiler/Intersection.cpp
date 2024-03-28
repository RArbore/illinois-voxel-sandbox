#include <bit>
#include <sstream>

#include <utils/Assert.h>

#include "Compiler.h"

std::string generate_intersection_glsl(const std::vector<InstantiatedFormat> &format, const Optimizations &opt) {
    std::stringstream ss;
    
    auto [total_w, total_h, total_d] = calculate_bounds(format);

    uint32_t df_bits_available = 0;
    if (opt.df_packing_) {
	uint64_t total_num_voxels = static_cast<uint64_t>(total_w) * static_cast<uint64_t>(total_h) * static_cast<uint64_t>(total_d);
	uint32_t saturated_total = total_num_voxels > 0xFFFFFFFF ? 0xFFFFFFFF : total_num_voxels;
	df_bits_available = std::countl_zero(saturated_total);
    }
    
    ss << R"(#version 460
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
        uvec3 rel_pos = voxel - low;
        bvec3 child_pos = greaterThanEqual(rel_pos, uvec3(diff / 2));
        uint child_idx = uint(child_pos.x) + 2 * uint(child_pos.y) + 4 * uint(child_pos.z);
        uint child_node_id = voxel_buffers[volume_id].voxels[node_id + child_idx];
        if (child_node_id != 0) {
            if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                return voxel_buffers[volume_id].voxels[child_node_id];
            }
        } else {
            return 0;
        }
	node_id = child_node_id;
        low += ivec3(uvec3(child_pos) * diff / 2);
        diff /= 2;
    }
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
	case Format::DF: {
	    bool do_df_compression = (1 << df_bits_available) >= format[i].parameters_[3] && static_cast<uint32_t>(i + 1) < format.size();
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
)";
	    if (do_df_compression) {
		ss << R"(            uint32_t child_id = voxel_buffers[volume_id].voxels[node_id + voxel_index] & )" << (static_cast<uint32_t>(0xFFFFFFFF) >> df_bits_available) << R"(;
            budget = (voxel_buffers[volume_id].voxels[node_id + voxel_index] >> )" << (32 - df_bits_available) << R"() + 1;
)";
	    } else {
		ss << R"(            uint32_t child_id = voxel_buffers[volume_id].voxels[node_id + 2 * voxel_index];
            budget = voxel_buffers[volume_id].voxels[node_id + 2 * voxel_index + 1] + 1;
)";
	    }
	    ss << R"(            
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
	}
	case Format::SVO:
	    if (!opt.unroll_sv_ && !opt.restart_sv_) {
		ss << R"(    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    StackFrame stack[)" << (format[i].parameters_[0] + 1) << R"(];
    stack[0].low = first_low;
    stack[0].curr_node_id = node_id;
    stack[0].left_off = 0;
    int level = 0;
    while (level >= 0 && level < )" << format[i].parameters_[0] << R"() {
        StackFrame stack_frame = stack[level];
        vec3 low = stack_frame.low;
        uint curr_node_id = stack_frame.curr_node_id;
        uint curr_node_child = voxel_buffers[volume_id].voxels[curr_node_id];
        uint curr_node_masks = voxel_buffers[volume_id].voxels[curr_node_id + 1];
        uint left_off = stack_frame.left_off;
        vec3 diff = vec3(sub_w, sub_h, sub_d) * float(1 << ()" << format[i].parameters_[0] << R"( - level));
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
                        if (intersect_format_)" << i + 1 << R"((volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
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
)";
	    } else if (!opt.unroll_sv_) {
		ss << R"(    vec3 chunk_ray_pos = (obj_ray_pos - lower) / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(o_t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3()" << (this_w - 1) << R"(, )" <<  (this_h - 1) << R"(, )" << (this_d - 1) << R"()));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    while (all(greaterThanEqual(chunk_ray_voxel, ivec3(0))) && all(lessThan(chunk_ray_voxel, ivec3()" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"()))) {
        uint32_t child_id = lookup_utility_restart_svo(volume_id, node_id, )" << format[i].parameters_[0] << R"(, chunk_ray_voxel);
        
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
	    } else {
		ss << R"(    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 low_0 = lower;
    uint curr_node_id_0 = node_id;
)";
		for (uint32_t j = 0; j < format[i].parameters_[0]; ++j) {
		    ss << R"(    vec3 diff_)" << j << R"( = vec3(sub_w, sub_h, sub_d) * float(1 << ()" << format[i].parameters_[0] << R"( - )" << j << R"());
    uint children_node_id = voxel_buffers[volume_id].voxels[curr_node_id_)" << j << R"(];
    uint children_masks = voxel_buffers[volume_id].voxels[curr_node_id_)" << j << R"( + 1];
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint valid = (children_masks >> (7 - child)) & 1;
        if (bool(valid)) {
            vec3 sub_low = low_)" << j << R"( + subpositions(child) * diff_)" << j << R"( * 0.5;
            vec3 sub_high = sub_low + diff_)" << j << R"( * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                uint leaf = (children_masks >> (15 - child)) & 1;
                uint num_valid = bitCount((children_masks & 0xFF) >> (8 - child));
                uint child_node_id = children_node_id + num_valid * 2;
                if (bool(leaf)) {
                    if (intersect_format_)" << i + 1 << R"((volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_)" << (j + 1) << R"( = child_node_id;
                    vec3 low_)" << (j + 1) << R"( = sub_low;

)";
		}
		
		for (uint32_t j = 0; j < format[i].parameters_[0]; ++j) {
		    ss << R"(                }
            }
        }
    }
)";
		}
		ss << R"(    return false;
)";
	    }
	    break;
	case Format::SVDAG:
	    if (!opt.unroll_sv_ && !opt.restart_sv_) {
		ss << R"(    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 first_low = lower;
    StackFrame stack[)" << (format[i].parameters_[0] + 1) << R"(];
    stack[0].low = first_low;
    stack[0].curr_node_id = node_id;
    stack[0].left_off = 0;
    int level = 0;
    while (level >= 0 && level < )" << format[i].parameters_[0] << R"() {
        StackFrame stack_frame = stack[level];
        vec3 low = stack_frame.low;
        uint curr_node_id = stack_frame.curr_node_id;
        uint left_off = stack_frame.left_off;
        vec3 diff = vec3(sub_w, sub_h, sub_d) * float(1 << ()" << format[i].parameters_[0] << R"( - level));
        for (uint idx = left_off; idx < 8; ++idx) {
            uint child = direction_kind ^ idx;
            uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id + child];
            if (child_node_id != 0) {
                vec3 sub_low = low + subpositions(child) * diff * 0.5;
                vec3 sub_high = sub_low + diff * 0.5;
                aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
                if (hit.front_t != -FAR_AWAY) {
                    if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                        if (intersect_format_)" << i + 1 << R"((volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
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
)";
	    } else if (!opt.unroll_sv_) {
		ss << R"(    vec3 chunk_ray_pos = (obj_ray_pos - lower) / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(o_t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3()" << (this_w - 1) << R"(, )" <<  (this_h - 1) << R"(, )" << (this_d - 1) << R"()));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    while (all(greaterThanEqual(chunk_ray_voxel, ivec3(0))) && all(lessThan(chunk_ray_voxel, ivec3()" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"()))) {
        uint32_t child_id = lookup_utility_restart_svdag(volume_id, node_id, )" << format[i].parameters_[0] << R"(, chunk_ray_voxel);
        
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
	    } else {
		ss << R"(    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);
    vec3 low_0 = lower;
    uint curr_node_id_0 = node_id;
)";
		for (uint32_t j = 0; j < format[i].parameters_[0]; ++j) {
		    ss << R"(    vec3 diff_)" << j << R"( = vec3(sub_w, sub_h, sub_d) * float(1 << ()" << format[i].parameters_[0] << R"( - )" << j << R"());
    for (uint idx = 0; idx < 8; ++idx) {
        uint child = direction_kind ^ idx;
        uint child_node_id = voxel_buffers[volume_id].voxels[curr_node_id_)" << j << R"( + child];
        if (child_node_id != 0) {
            vec3 sub_low = low_)" << j << R"( + subpositions(child) * diff_)" << j << R"( * 0.5;
            vec3 sub_high = sub_low + diff_)" << j << R"( * 0.5;
            aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
            if (hit.front_t != -FAR_AWAY) {
                if (voxel_buffers[volume_id].voxels[child_node_id + 1] == 0xFFFFFFFF) {
                    if (intersect_format_)" << i + 1 << R"((volume_id, voxel_buffers[volume_id].voxels[child_node_id], obj_ray_pos, obj_ray_dir, sub_low, hit.front_t, hit.k)) {
                        return true;
                    }
                } else {
                    uint curr_node_id_)" << (j + 1) << R"( = child_node_id;
                    vec3 low_)" << (j + 1) << R"( = sub_low;

)";
		}
		
		for (uint32_t j = 0; j < format[i].parameters_[0]; ++j) {
		    ss << R"(                }
            }
        }
    }
)";
		}
		ss << R"(    return false;
)";
	    }
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
