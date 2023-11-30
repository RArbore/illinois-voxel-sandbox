#version 460
#pragma shader_stage(compute)

layout (binding = 0, rgba8) uniform writeonly image2D output_image;
layout (binding = 1, rgba8) uniform readonly image2D noisy_image;
layout (binding = 2, rgba8) uniform readonly image2D image_normals;
layout (binding = 3, rgba8) uniform readonly image2D image_positions;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Referenced from https://64.github.io/tonemapping/#extended-reinhard-luminance-tone-map
float luminance(vec3 color) {
    return dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 initial_color, float final_luminance) {
    float initial_luminance = luminance(initial_color);
    return initial_color * (final_luminance / initial_luminance);
}

// Reinhard extended luminance
vec3 reinhard(vec3 color) {
    const float white_point = 1.0f;

    float initial_luminance = luminance(color);
    float factor = initial_luminance * (1.0f + (initial_luminance / (white_point * white_point)));
    float final_luminance = factor / (1.0f + initial_luminance);
    return change_luminance(color, final_luminance);
}

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    vec3 pixel_color = imageLoad(noisy_image, pixel).xyz;
    pixel_color = clamp(reinhard(pixel_color), 0.0, 1.0);
    imageStore(output_image, pixel, vec4(pixel_color, 1.0f));
}