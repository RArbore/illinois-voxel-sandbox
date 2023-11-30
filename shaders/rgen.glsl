#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "sample_util.glsl"
#include "voxel_bsdf.glsl"

layout(location = 0) rayPayloadEXT RayPayload payload;

const int MAX_BOUNCES = 3;

uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

vec2 slice_2_from_4(vec4 random, uint num) {
    uint slice = (num + camera.frames_since_update) % 4;
    return random.xy * float(slice == 0) + random.yz * float(slice == 1) + random.zw * float(slice == 2) + random.wx * float(slice == 3);
}

void main() {
    // Setup and shoot the camera ray first
    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + 0.5;
    const vec2 pixel_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    const vec2 pixel_ndc = 2.0f * pixel_uv - 1.0f;
    vec3 ray_origin = (camera.view_inv * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    vec3 ray_direction = normalize((camera.view_inv * vec4(pixel_ndc.xy, 1.0f, 0.0)).xyz);

    uint t = camera.frames_since_update + uint(elapsed_ms);

    uvec2 blue_noise_size = imageSize(blue_noise);
    ivec2 blue_noise_coords = ivec2((gl_LaunchIDEXT.xy + ivec2(hash(t), hash(3 * t))) % blue_noise_size);
    vec4 random = imageLoad(blue_noise, blue_noise_coords);

    vec3 L = vec3(0.0f);
    vec3 weight = vec3(1.0f);
    int bounce = 0;
    for (bounce = 0; bounce < MAX_BOUNCES; bounce++) {
        traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_origin, 0.001f, ray_direction, 10000.0f, 0);

        if (payload.hit && payload.emissive) {
            L += weight * payload.color.xyz * payload.color.w * 255.0; // if we hit a direct light
        }

        // If we've hit something, we send another ray in a random direction.
        // Otherwise assume we hit the sky.
        // Note that this is **only** doing indirect lighting.
        if (payload.hit) {
            if (bounce == 0) {
                imageStore(image_normals, ivec2(gl_LaunchIDEXT), vec4(payload.world_normal, 1.0f));
                imageStore(image_positions, ivec2(gl_LaunchIDEXT), vec4(payload.world_position, 1.0f));
            }

            // Choose a new direction and evaluate the BRDF/PDF
            vec3 new_local_direction = cosine_sample_hemisphere(slice_2_from_4(random, bounce));
            float pdf = dot(vec3(0.0f, 1.0f, 0.0f), new_local_direction) * INV_PI; // cosine-weighted sampling
            if (pdf < 0.001) {
                break;
            }
            vec3 bsdf = payload.color.xyz * INV_PI;

            // Set up the new ray
            vec3 new_direction = local_to_world(new_local_direction, payload.world_normal, payload.tangent, payload.bitangent);
            ray_origin = payload.world_position + 0.01 * payload.world_normal;
            ray_direction = new_direction;
            weight *= bsdf * abs(dot(new_direction, payload.world_normal)) / pdf;
        } else {
            L += weight * payload.color.xyz; // add the sky color if the first ray is a miss
            break;
        }
    }

    vec4 final_radiance = vec4(L, 1.0f);

    // imageStore(image_history, ivec2(gl_LaunchIDEXT), final_radiance);

    if (camera.frames_since_update == 0) {
        imageStore(image_history, ivec2(gl_LaunchIDEXT), final_radiance);
    } else {
        vec4 history = imageLoad(image_history, ivec2(gl_LaunchIDEXT));
	    float proportion = max(1.0f / (camera.frames_since_update + 1), 0.1);
        vec4 final_color = mix(history, final_radiance, proportion);
        imageStore(image_history, ivec2(gl_LaunchIDEXT), final_color);
    }
}
