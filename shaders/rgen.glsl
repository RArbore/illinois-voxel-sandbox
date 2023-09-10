#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadEXT vec4 hit;

void main() {
    float t = float(elapsed_ms) / 1000.0;

    vec2 normalized = vec2(gl_LaunchIDEXT.xy) / vec2(gl_LaunchSizeEXT.xy) - 0.5;
    vec3 ray_origin = 10.0 * vec3(cos(t), 0.0, sin(t));
    vec3 ray_direction = normalize(normalize(vec3(0.5) - ray_origin) + vec3(-normalized.x * sin(t), normalized.y, normalized.x * cos(t)));

    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_origin, 0.001, ray_direction, 10000.0, 0);

    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), hit);
}
