#include <sstream>

#include <utils/Assert.h>

#include "Compiler.h"

std::string generate_intersection_glsl(const std::vector<InstantiatedFormat> &format) {
    std::stringstream ss;
    
    auto [total_w, total_h, total_d] = calculate_bounds(format);
    
    ss << R"(#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

bool intersect_format_)" << format.size() << R"((uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, float t) {
    leaf_id = node_id;
    reportIntersectionEXT(t, 0);
    return true;
}
)";

    for (int32_t i = format.size() - 1; i >= 0; --i) {
	ss << R"(
bool intersect_format_)" << i << R"((uint volume_id, uint node_id, vec3 obj_ray_pos, vec3 obj_ray_dir, float t) {
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
	    ss << R"(    vec3 chunk_ray_pos = obj_ray_pos / vec3(sub_w, sub_h, sub_d);
    vec3 chunk_ray_dir = obj_ray_dir;
    vec3 chunk_ray_intersect_point = chunk_ray_pos + chunk_ray_dir * max(t, 0.0) / vec3(sub_w, sub_h, sub_d);
    ivec3 chunk_ray_voxel = ivec3(min(chunk_ray_intersect_point, ivec3()" << (this_w - 1) << R"(, )" <<  (this_h - 1) << R"(, )" << (this_d - 1) << R"()));
    ivec3 chunk_ray_step = ivec3(sign(chunk_ray_dir));
    vec3 chunk_ray_delta = abs(vec3(length(chunk_ray_dir)) / chunk_ray_dir);
    vec3 chunk_side_dist = (sign(chunk_ray_dir) * (vec3(chunk_ray_voxel) - chunk_ray_intersect_point) + (sign(chunk_ray_dir) * 0.5) + 0.5) * chunk_ray_delta;

    while (all(greaterThanEqual(obj_ray_voxel, ivec3(0))) && all(lessThan(obj_ray_voxel, ivec3()" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"()))) {
        uint32_t voxel_index = linearize_index(chunk_ray_voxel, )" << this_w << R"(, )" << this_h << R"(, )" << this_d << R"();
        uint32_t child_id = voxel_buffers[volume_id][node_id + voxel_index];

        if (palette != 0) {
            aabb_intersect_result r = hit_aabb(chunk_ray_voxel, chunk_ray_voxel + 1, chunk_ray_pos, chunk_ray_dir);
            float intersect_time = length(gl_ObjectToWorldEXT * vec4(chunk_ray_pos + chunk_ray_dir * r.front_t, 1.0) - gl_ObjectToWorldEXT * vec4(chunk_ray_pos, 1.0));
            if (intersect_format_)" << i + 1 << R"((volume_id, child_id, obj_ray_pos, obj_ray_dir, intersect_time)) {
                return true;
            }
        }

        bvec3 mask = lessThanEqual(chunk_side_dist.xyz, min(chunk_side_dist.yzx, chunk_side_dist.zxy));
        chunk_side_dist += vec3(mask) * chunk_ray_delta;
        chunk_ray_voxel += ivec3(mask) * chunk_ray_step;
    }
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
    uint root_id = voxel_buffers[volume_id][0];

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3()" << total_w << R"(.0, )" << total_h << R"(.0, )" << total_d << R"(.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
        intersect_format_0(volume_id, root_id, obj_ray_pos, obj_ray_dir, r.front_t);
    }
}
)";
    
    return ss.str();
}
