#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadEXT vec4 hit;

void main() {
    float t = float(elapsed_ms) / 1000.0;

    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + 0.5;
    const vec2 pixel_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    const vec2 pixel_ndc = 2.0f * pixel_uv - 1.0f;

    vec3 origin = (camera.view_inv * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    vec3 direction = normalize(camera.view_inv * vec4(pixel_ndc.xy, 1.0f, 0.0)).xyz);

    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin, 0.001f, direction, 10000.0f, 0);

    // To-do: add accumulation if the camera hasn't moved
    imageStore(output_image, ivec2(gl_LaunchIDEXT), hit);
}
