#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_atomic_int64 : require

struct SVONode {
    uint32_t child_pointer_;
    uint8_t valid_mask_;
    uint8_t leaf_mask_;
    uint8_t _padding_[2];
};

struct SVOLeaf_Color {
    uint8_t red_;
    uint8_t green_;
    uint8_t blue_;
    uint8_t alpha_;
    uint8_t _padding_[4];
};

layout (push_constant) uniform PushConstants {
    uint64_t elapsed_ms;
};

struct RayPayload {
    vec4 color;
    vec3 world_position;
    vec3 world_normal;
    bool hit;
};

// Set 0 is swapped out per swapchain image.
layout(set = 0, binding = 0, rgba8) uniform image2D output_image;

// Set 1 is swapped out per scene.
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 1, binding = 1, rgba8) uniform readonly image3D volumes[];
layout(set = 1, binding = 2) buffer SVOBuffer {
    uint32_t voxel_width;
    uint32_t voxel_height;
    uint32_t voxel_depth;
    uint32_t num_nodes;
    SVONode nodes[];
} svo_buffers[];
layout(set = 1, binding = 2) buffer SVOBuffer_Color {
    uint32_t voxel_width;
    uint32_t voxel_height;
    uint32_t voxel_depth;
    uint32_t num_nodes;
    SVOLeaf_Color nodes[];
} svo_leaf_color_buffers[];

// Set 2 is not swapped out - it is for GraphicsContext-wide data.
#define MAX_NUM_CHUNKS_LOADED_PER_FRAME 32
layout(set = 2, binding = 0) buffer chunk_request_buffer_ {
    uint64_t chunk_request_buffer[];
};

struct Camera {
    mat4 view_inv; // view space to world space
    int frames_since_update; // accumulated frames since the camera hasn't moved
};

layout(set = 2, binding = 1) uniform camera_ {
	Camera camera;
};

layout(set = 2, binding = 2, rgba8) uniform image2D image_history;

const float FAR_AWAY = 1000.0;
