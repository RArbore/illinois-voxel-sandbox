#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_atomic_int64 : require

layout (push_constant) uniform PushConstants {
    uint64_t elapsed_ms;
};

// Set 0 is swapped out per swapchain image.
layout(set = 0, binding = 0, rgba8) uniform image2D output_image;

// Set 1 is swapped out per scene.
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 1, binding = 1, rgba8) uniform readonly image3D volumes[];

// Set 2 is not swapped out - it is for GraphicsContext-wide data.
#define MAX_NUM_CHUNKS_LOADED_PER_FRAME 32
layout(set = 2, binding = 0) buffer chunk_request_buffer_ {
    uint64_t chunk_request_buffer[];
};

struct Camera {
    mat4 view_inv; // view space to world space
}

layout(set = 2, binding = 1) uniform Camera camera;

const float FAR_AWAY = 1000.0;
