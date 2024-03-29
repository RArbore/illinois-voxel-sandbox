#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "voxel_bsdf.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT uint leaf_id;

uint pcg_hash(uint seed) {
  uint state = seed * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    uint32_t color = leaf_id;
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
