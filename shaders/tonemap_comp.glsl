#version 460
#pragma shader_stage(compute)

#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require

layout (push_constant) uniform params {
    // 0 is Reinhard, 1 is ACES
    uint16_t tonemap_option;
};

layout (binding = 0, rgba8) uniform writeonly image2D output_image;
layout (binding = 1, rgba16f) uniform readonly image2D denoised_image;

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

vec3 aces_approx(vec3 color)
{
    color *= 0.6f;
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return (color * (a * color + b)) / (color *(c * color + d) + e);
}

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    vec3 pixel_color = imageLoad(denoised_image, pixel).xyz;
    if (tonemap_option == 0) {
        pixel_color = clamp(reinhard(pixel_color), 0.0f, 1.0f);
    } else if (tonemap_option == 1) {
        pixel_color = clamp(aces_approx(pixel_color), 0.0f, 1.0f);
    } else {
        pixel_color = vec3(1.0f);
    }
    imageStore(output_image, pixel, vec4(pixel_color, 1.0f));
}