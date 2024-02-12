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

	switch (format[i].format_) {
	case Format::Raw:
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
