#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "voxel_bsdf.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT uint leaf_id;

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    uint32_t color = 0xFF0000FF;//voxel_buffers[volume_id].voxels[leaf_id];
    payload.hit = true;
    payload.world_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    payload.world_normal = normalize(gl_ObjectToWorldEXT * vec4(voxel_normals[gl_HitKindEXT], 0.0));
    payload.tangent = normalize(gl_ObjectToWorldEXT * vec4(voxel_tangents[gl_HitKindEXT], 0.0));
    payload.bitangent = normalize(gl_ObjectToWorldEXT * vec4(voxel_bitangents[gl_HitKindEXT], 0.0));
    payload.color = vec4(
	       float(color & 0xFF) / 255.0,
	       float((color >> 8) & 0xFF) / 255.0,
	       float((color >> 16) & 0xFF) / 255.0,
	       float(color >> 24) / 255.0
	       );
    payload.voxel_face = gl_HitKindEXT;
    payload.emissive = false;
}
