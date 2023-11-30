#version 460
#pragma shader_stage(raygen)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "sample_util.glsl"
#include "voxel_bsdf.glsl"

layout(location = 0) rayPayloadEXT RayPayload payload;

const int MAX_BOUNCES = 2;

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
        
        // If we've hit something, we send another ray in a random direction.
        // Otherwise assume we hit the sky. Todo: toggle the sky as an infinite light
        if (payload.hit) {
            RayPayload isect = payload;

            if (bounce == 0 && !bool(download_bit)) {
                imageStore(image_normals, ivec2(gl_LaunchIDEXT), vec4(isect.world_normal, 1.0f));
                imageStore(image_positions, ivec2(gl_LaunchIDEXT), vec4(isect.world_position, 1.0f));

                if (isect.emissive) {
                    L += weight * isect.color.xyz;
                }
            }

            // Choose a random light to sample to 
            if (num_emissive_voxels > 0) {
                vec4 random_light = imageLoad(blue_noise, blue_noise_coords + ivec2(bounce * slice_2_from_4(random, bounce + 3)));
                uint32_t light_index = uint32_t(num_emissive_voxels * random_light.x); // pick a light
                uint8_t light_face = uint8_t(6 * random_light.y);

                vec3 light_corner = vec3(emissive_voxels[6 * light_index + 0], 
                                         emissive_voxels[6 * light_index + 1], 
                                         emissive_voxels[6 * light_index + 2]);
                
                vec3 light_point = light_corner;
                float light_pdf = 1.0f / (6 * num_emissive_voxels);
                float width = emissive_voxels[6 * light_index + 3];
                float height = emissive_voxels[6 * light_index + 4];
                float depth = emissive_voxels[6 * light_index + 5];
                if (light_face == 0) {
                    light_point += vec3(width * random_light.z, height * random_light.w, 0);
                    light_pdf /= (width * height);
                } else if (light_face == 1) {
                    light_point += vec3(0, height * random_light.z, depth * random_light.w);
                    light_pdf /= (height * depth);
                } else if (light_face == 2) {
                    light_point += vec3(width * random_light.z, 0, depth * random_light.w);
                    light_pdf /= (width * depth);
                } else if (light_face == 3) {
                    light_point += vec3(width * random_light.z, height * random_light.w, depth);
                    light_pdf /= (width * height);
                } else if (light_face == 4) {
                    light_point += vec3(width, height * random_light.z, depth * random_light.w);
                    light_pdf /= (height * depth);
                } else if (light_face == 5) {
                    light_point += vec3(width * random_light.z, height, depth * random_light.w);
                    light_pdf /= (width * depth);
                }

                vec3 light_ray_origin = isect.world_position + 0.01 * isect.world_normal;
                vec3 light_direction = normalize(light_point - light_ray_origin);
                traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, light_ray_origin, 0.001f, light_direction, 10000.0f, 0);

                if (payload.hit && length(payload.world_position - light_point) < 0.001) {
                    vec3 bsdf = isect.color.xyz * INV_PI;
                    L += weight * payload.color.xyz * bsdf * payload.color.w / light_pdf;
                }
            }

            // Choose a new direction and evaluate the BRDF/PDF
            vec3 new_local_direction = cosine_sample_hemisphere(slice_2_from_4(random, bounce));
            float pdf = dot(vec3(0.0f, 1.0f, 0.0f), new_local_direction) * INV_PI; // cosine-weighted sampling
            if (pdf < 0.001) {
                break;
            }
            vec3 bsdf = isect.color.xyz * INV_PI;

            // Set up the new ray
            vec3 new_direction = local_to_world(new_local_direction, isect.world_normal, isect.tangent, isect.bitangent);
            ray_origin = isect.world_position + 0.01 * isect.world_normal;
            ray_direction = new_direction;
            weight *= bsdf * abs(dot(new_direction, isect.world_normal)) / pdf;
        } else {
            L += weight * payload.color.xyz; // add the sky color if the first ray is a miss
            break;
        }
    }

    vec4 final_radiance = vec4(L, 1.0f);

    if (!bool(download_bit)) {
        if (camera.frames_since_update == 0) {
            imageStore(image_history, ivec2(gl_LaunchIDEXT), final_radiance);
        } else {
            vec4 history = imageLoad(image_history, ivec2(gl_LaunchIDEXT));
            float proportion = max(1.0f / (camera.frames_since_update + 1), 0.1);
            vec4 final_color = mix(history, final_radiance, proportion);
            imageStore(image_history, ivec2(gl_LaunchIDEXT), final_color);
        }
    }
}
