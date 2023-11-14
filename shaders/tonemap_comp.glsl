#version 460
#pragma shader_stage(compute)

layout (binding = 0, rgba8) uniform writeonly image2D output_image;
layout (binding = 1, rgba8) uniform readonly image2D noisy_image;
layout (binding = 2, rgba8) uniform readonly image2D image_normals;
layout (binding = 3, rgba8) uniform readonly image2D image_positions;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{
    // Just a clamp for the time being, 
    // but can be used for denoising + tonemapping in the future.
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    vec4 pixel_color = imageLoad(noisy_image, pixel);
    pixel_color = clamp(pixel_color, 0.0, 1.0);
    imageStore(output_image, pixel, pixel_color);
}