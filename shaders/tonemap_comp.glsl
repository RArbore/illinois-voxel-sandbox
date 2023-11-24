#version 460
#pragma shader_stage(compute)

layout (binding = 0, rgba8) uniform writeonly image2D output_image;
layout (binding = 1, rgba8) uniform readonly image2D noisy_image;
layout (binding = 2, rgba8) uniform readonly image2D image_normals;
layout (binding = 3, rgba8) uniform readonly image2D image_positions;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Referenced from https://github.com/NVIDIAGameWorks/Falcor/blob/master/Source/RenderPasses/ToneMapper/ToneMapping.ps.slang
float color_luminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 reinhard(vec3 color) {
    float luminance = color_luminance(color);
    return color / (luminance + 1);
}

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    vec3 pixel_color = imageLoad(noisy_image, pixel).xyz;
    pixel_color = clamp(reinhard(pixel_color), 0.0, 1.0);
    imageStore(output_image, pixel, vec4(pixel_color, 1.0f));
}