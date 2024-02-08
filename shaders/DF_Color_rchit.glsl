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
    uint voxel_index = linearize_index(volume_load_pos, raw_buffers[volume_id].voxel_width, raw_buffers[volume_id].voxel_height, raw_buffers[volume_id].voxel_depth); 

    payload.hit = true;
    payload.world_position = world_ray_pos;
    payload.world_normal = normalize(gl_ObjectToWorldEXT * vec4(voxel_normals[gl_HitKindEXT], 0.0));
    payload.tangent = normalize(gl_ObjectToWorldEXT * vec4(voxel_tangents[gl_HitKindEXT], 0.0));
    payload.bitangent = normalize(gl_ObjectToWorldEXT * vec4(voxel_bitangents[gl_HitKindEXT], 0.0));
    
    RawColor color = raw_buffers[volume_id].voxel_colors[voxel_index];
    payload.color = vec4(
	       float(int(color.red_)) / 255.0,
	       float(int(color.green_)) / 255.0,
	       float(int(color.blue_)) / 255.0,
	       1.0
	       );
    payload.voxel_face = gl_HitKindEXT;
    payload.emissive = false;
}
