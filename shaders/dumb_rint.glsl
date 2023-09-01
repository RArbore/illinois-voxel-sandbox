#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

void main() {
    reportIntersectionEXT(1.0, 0);
}
