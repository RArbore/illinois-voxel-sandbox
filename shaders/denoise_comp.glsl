#version 460
#pragma shader_stage(compute)

#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require

layout (push_constant) uniform params {
    uint32_t filter_level;
    double variance_rt;
    double variance_norm;
    double variance_pos;
};

layout (binding = 0, rgba16f) uniform writeonly image2D output_image;
layout (binding = 1, rgba16f) uniform readonly image2D noisy_image;
layout (binding = 2, rgba8) uniform readonly image2D image_normals;
layout (binding = 3, rgba8) uniform readonly image2D image_positions;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

const float kernel[25] = {
    1.0f/256.0f, 1.0f/64.0f, 3.0f/128.0f, 1.0f/64.0f, 1.0F/256.0f,
    1.0f/64.0f,  1.0f/16.0f, 3.0f/32.0f,  1.0f/16.0f, 1.0F/64.0f,
    3.0f/128.0f, 3.0f/32.0f, 9.0f/64.0f,  3.0f/32.0f, 3.0F/128.0f,
    1.0f/64.0f,  1.0f/16.0f, 3.0f/32.0f,  1.0f/16.0f, 1.0F/64.0f,
    1.0f/256.0f, 1.0f/64.0f, 3.0f/128.0f, 1.0f/64.0f, 1.0F/256.0f   
};

const vec2 offsets[25] = {
    vec2(-2, -2), vec2(-1, -2), vec2(0, -2), vec2(1, -2), vec2(2, -2),
    vec2(-2, -1), vec2(-1, -1), vec2(0, -1), vec2(1, -1), vec2(2, -1),
    vec2(-2,  0), vec2(-1,  0), vec2(0,  0), vec2(1,  0), vec2(2,  0),
    vec2(-2,  1), vec2(-1,  1), vec2(0,  1), vec2(1,  1), vec2(2,  1),
    vec2(-2,  2), vec2(-1,  2), vec2(0,  2), vec2(1,  2), vec2(2,  2),
};

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    int step_size = 1 << filter_level;
    vec4 color = imageLoad(noisy_image, pixel);
    vec4 normal = imageLoad(image_normals, pixel);
    vec4 position = imageLoad(image_positions, pixel); 

    // vec4 sum = vec4(0.0f);
    // float k = 1.0f;
    // for (int i = 0; i < 25; i++) {
    //     ivec2 offset_pixel = pixel + ivec2(offsets[i] * step_size);
    //     if (all(greaterThanEqual(offset_pixel, ivec2(0))) && all(lessThan(offset_pixel, imageSize(noisy_image)))) {
    //         vec4 offset_color = imageLoad(noisy_image, offset_pixel);
    //         vec4 color_diff = color - offset_color;
    //         vec4 normal_diff = normal - imageLoad(image_normals, offset_pixel);
    //         vec4 position_diff = position - imageLoad(image_positions, offset_pixel);

    //         float w_rt = min(exp(-dot(color_diff, color_diff) / variance_rt), 1.0);
    //         float w_norm = min(exp(-max(dot(normal_diff, normal_diff) / (step_size * step_size), 0.0) / variance_norm), 1.0);
    //         float w_pos = min(exp(-dot(position_diff, position_diff) / variance_pos), 1.0);

    //         float h = kernel[i];
    //         float w = w_rt * w_norm * w_pos;

    //         k += h * w;
    //         sum += h * w * offset_color;
    //     }
    // }

    imageStore(output_image, pixel, color);
}
