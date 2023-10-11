#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT uint leaf_id;

void main() {
    uint svo_id = gl_InstanceCustomIndexEXT;
    SVOLeaf_Color color = svo_leaf_color_buffers[svo_id].nodes[leaf_id];
    payload.hit = false;
    payload.color = vec4(
	       float(int(color.red_)) / 255.0,
	       float(int(color.green_)) / 255.0,
	       float(int(color.blue_)) / 255.0,
	       float(int(color.alpha_)) / 255.0
	       );
}
