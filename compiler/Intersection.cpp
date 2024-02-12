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

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0));

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3()" << total_w << R"(.0, )" << total_h << R"(.0, )" << total_d << R"(.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
)";

    ss << R"(    }
}
)";

    return ss.str();
}
