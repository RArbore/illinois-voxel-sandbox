#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "sample_util.glsl"

layout(location = 0) rayPayloadEXT RayPayload payload;

void main() {
    float t = float(elapsed_ms);

    // Setup and shoot the camera ray first
    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + 0.5;
    const vec2 pixel_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    const vec2 pixel_ndc = 2.0f * pixel_uv - 1.0f;
    vec3 origin = (camera.view_inv * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    vec3 direction = normalize((camera.view_inv * vec4(pixel_ndc.xy, 1.0f, 0.0)).xyz);
    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin, 0.001f, direction, 10000.0f, 0);

    // Temporary accumulation test
    vec4 L = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (payload.hit) {
        // If we've hit something, we send another ray in a random direction.
        // If the next bounce hits the sky, we get the sky color.
        // Otherwise, we terminate the path to fake an ambient occlusion.
        vec4 color = payload.color;
    
        // Hack: super weird, not a great random number generator,
        // and we're sampling a unit sphere instead of local hemisphere,
        // so there's basically a 50% chance of getting a self-intersection.
        vec2 rand_uv = vec2(rand(vec2(pixel_uv * pixel_uv * t)), rand(vec2(pixel_uv * t * t)));
        rand_uv.x *= PI;
        rand_uv.y *= 2.0f * PI;

        vec3 new_direction = vec3(sin(rand_uv.x) * cos(rand_uv.y), cos(rand_uv.x), sin(rand_uv.x) * sin(rand_uv.y));
        vec3 new_origin = payload.world_position;
        traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, new_origin, 0.001f, new_direction, 10.0f, 0);

        //if (!payload.hit) {
            L = color;
        //}
    }
    L = payload.color;

    if (camera.frames_since_update == 0) {
        imageStore(image_history, ivec2(gl_LaunchIDEXT), L);
    } else {
        vec4 history = imageLoad(image_history, ivec2(gl_LaunchIDEXT));
        // vec4 final_color = mix(history, L, 1.0f / (camera.frames_since_update + 1));
        vec4 final_color = history;
        if (L.x > 0 || L.y > 0 || L.z > 0) {
            final_color = L;
        }
        imageStore(image_history, ivec2(gl_LaunchIDEXT), L);
    }
}
