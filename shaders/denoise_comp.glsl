#version 460
#pragma shader_stage(compute)

layout (push_constant) uniform params {
    uint filter_level;
    float variance_rt;
    float variance_norm;
    float variance_pos;
};

layout (binding = 0, rgba16f) uniform writeonly image2D output_image;
layout (binding = 1, rgba16f) uniform readonly image2D noisy_image;
layout (binding = 2, rgba8) uniform readonly image2D image_normals;
layout (binding = 3, rgba8) uniform readonly image2D image_positions;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

float blur_kernel_3x3[9] = float[9](
				    1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0,
				    1.0 / 8.0, 1.0 / 4.0, 1.0 / 8.0,
				    1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0
				    );

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    int step_size = 1 << filter_level;
    vec4 color = imageLoad(noisy_image, pixel);
    vec4 normal = imageLoad(image_normals, pixel);
    vec4 position = imageLoad(image_positions, pixel); 

    vec4 sum = vec4(0.0f);
    float k = 1.0f;
    for (int i = 0; i < 9; i++) {
        ivec2 offset_pixel = pixel + step_size * ivec2(i % 3 - 1, i / 3 - 1);
        if (all(greaterThanEqual(offset_pixel, ivec2(0))) && all(lessThan(offset_pixel, imageSize(noisy_image)))) {
            vec4 offset_color = imageLoad(noisy_image, offset_pixel);
            vec4 color_diff = color - offset_color;
            vec4 normal_diff = normal - imageLoad(image_normals, offset_pixel);
            vec4 position_diff = position - imageLoad(image_positions, offset_pixel);

            float w_norm = min(exp(-max(dot(normal_diff, normal_diff) / (step_size * step_size), 0.0) / variance_norm), 1.0);
            float w_pos = min(exp(-dot(position_diff, position_diff) / variance_pos), 1.0);

            float h = blur_kernel_3x3[i];
            float w = w_norm * w_pos;

            k += h * w;
            sum += h * w * offset_color;
        }
    }

    imageStore(output_image, pixel, color);
}
