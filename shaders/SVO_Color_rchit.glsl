#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "voxel_bsdf.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT uint leaf_id;
hitAttributeEXT uint voxel_normal_id;

void main() {
    uint svo_id = gl_InstanceCustomIndexEXT;
    SVOLeaf_Color color = svo_leaf_color_buffers[svo_id].nodes[leaf_id];
    payload.hit = true;
    payload.world_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    payload.world_normal = gl_ObjectToWorldEXT * vec4(voxel_normals[gl_HitKindEXT], 0.0);
    payload.tangent = gl_ObjectToWorldEXT * vec4(voxel_tangents[gl_HitKindEXT], 0.0);
    payload.bitangent = gl_ObjectToWorldEXT * vec4(voxel_bitangents[gl_HitKindEXT], 0.0);
    payload.color = vec4(
	       float(int(color.red_)) / 255.0,
	       float(int(color.green_)) / 255.0,
	       float(int(color.blue_)) / 255.0,
	       float(int(color.alpha_)) / 255.0
	       );
    payload.voxel_face = gl_HitKindEXT;
}
