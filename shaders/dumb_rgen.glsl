#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0, rgba8) uniform image2D output_image;
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadEXT vec4 hit;

void main() {
    vec2 normalized = vec2(gl_LaunchIDEXT.xy) / vec2(gl_LaunchSizeEXT.xy);

    vec3 ray_origin = vec3(normalized * 2.0, -2.0);
    vec3 ray_direction = vec3(0.0, 0.0, 1.0);
    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_origin, 0.001, ray_direction, 10000.0, 0);

    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), hit);
}
