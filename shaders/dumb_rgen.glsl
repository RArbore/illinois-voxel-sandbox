#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout (push_constant) uniform PushConstants {
    uint64_t frame_index;
};

layout(set = 0, binding = 0, rgba8) uniform image2D output_image;
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadEXT vec4 hit;

void main() {
    float t = float(frame_index) / 100.0;

    vec2 normalized = vec2(gl_LaunchIDEXT.xy) / vec2(gl_LaunchSizeEXT.xy) - 0.5;
    vec3 ray_origin = 3.0 * vec3(cos(t), 0.0, sin(t));
    vec3 ray_direction = normalize(normalize(vec3(0.5) - ray_origin) + vec3(-normalized.x * sin(t), normalized.y, normalized.x * cos(t)));

    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_origin, 0.001, ray_direction, 10000.0, 0);

    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), hit);
}
