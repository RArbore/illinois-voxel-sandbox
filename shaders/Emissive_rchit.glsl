#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "voxel_bsdf.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;

    vec3 world_ray_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 world_obj_pos = gl_ObjectToWorldEXT * vec4(0.0, 0.0, 0.0, 1.0);

    vec3 voxel_sample_pos = gl_WorldToObjectEXT * vec4(world_ray_pos, 1.0);
    ivec3 volume_load_pos = ivec3(voxel_sample_pos - 0.5 * voxel_normals[gl_HitKindEXT]);

    payload.hit = true;
    payload.world_position = world_ray_pos;
    payload.world_normal = gl_ObjectToWorldEXT * vec4(voxel_normals[gl_HitKindEXT], 0.0);
    payload.tangent = gl_ObjectToWorldEXT * vec4(voxel_tangents[gl_HitKindEXT], 0.0);
    payload.bitangent = gl_ObjectToWorldEXT * vec4(voxel_bitangents[gl_HitKindEXT], 0.0);
    payload.color = imageLoad(volumes[volume_id], volume_load_pos);
    payload.voxel_face = gl_HitKindEXT;
    payload.emissive = true;
}