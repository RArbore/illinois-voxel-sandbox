#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout (push_constant) uniform PushConstants {
    uint64_t elapsed_ms;
};

layout(set = 0, binding = 0, rgba8) uniform image2D output_image;

layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 1, binding = 1, rgba8) uniform readonly image3D volumes[];

const float FAR_AWAY = 1000.0;
