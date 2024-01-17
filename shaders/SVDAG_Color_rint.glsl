#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

#define MAX_DEPTH 16

uint round_up_p2(uint x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    return x;
}

/*
const vec3 subpositions[8] = vec3[8](
				     vec3(0.0, 0.0, 0.0),
				     vec3(1.0, 0.0, 0.0),
				     vec3(0.0, 1.0, 0.0),
				     vec3(1.0, 1.0, 0.0),
				     vec3(0.0, 0.0, 1.0),
				     vec3(1.0, 0.0, 1.0),
				     vec3(0.0, 1.0, 1.0),
				     vec3(1.0, 1.0, 1.0)
				     );
*/

vec3 subpositions(uint child) {
    return vec3(
		bool(child % 2) ? 1.0 : 0.0,
		bool((child / 2) % 2) ? 1.0 : 0.0,
		bool((child / 4) % 2) ? 1.0 : 0.0
		);
}

/*
const uint children_iteration_order[8][8] = uint[8][8](
						       uint[8](0, 1, 2, 3, 4, 5, 6, 7), = child ^ 00000000
						       uint[8](1, 0, 3, 2, 5, 4, 7, 6), = child ^ 00000001
						       uint[8](2, 3, 0, 1, 6, 7, 4, 5), = child ^ 00000010
						       uint[8](3, 2, 1, 0, 7, 6, 5, 4), = child ^ 00000011
						       uint[8](4, 5, 6, 7, 0, 1, 2, 3), = child ^ 00000100
						       uint[8](5, 4, 7, 6, 1, 0, 3, 2), = child ^ 00000101
						       uint[8](6, 7, 4, 5, 2, 3, 0, 1), = child ^ 00000110
						       uint[8](7, 6, 5, 4, 3, 2, 1, 0)  = child ^ 00000111
						       );
*/

uint children_iteration_order(uint kind, uint child) {
    return child ^ kind;
}

struct SVDAGMarchStackFrame {
    vec4 info;
};

void main() {
    uint svdag_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);

    vec3 first_low = vec3(
			  primitive_dimensions[svdag_id].primitive_aabbs[6 * gl_PrimitiveID],
			  primitive_dimensions[svdag_id].primitive_aabbs[6 * gl_PrimitiveID + 1],
			  primitive_dimensions[svdag_id].primitive_aabbs[6 * gl_PrimitiveID + 2]
			  );
    vec3 first_high = vec3(
			  primitive_dimensions[svdag_id].primitive_aabbs[6 * gl_PrimitiveID + 3],
			  primitive_dimensions[svdag_id].primitive_aabbs[6 * gl_PrimitiveID + 4],
			  primitive_dimensions[svdag_id].primitive_aabbs[6 * gl_PrimitiveID + 5]
			  );
    aabb_intersect_result bounding = hit_aabb(first_low, first_high, obj_ray_pos, obj_ray_dir);
    if (bounding.front_t != -FAR_AWAY) {
	int level = 0;
	SVDAGMarchStackFrame stack[MAX_DEPTH];
	stack[level].info = vec4(0.0, 0.0, 0.0, uintBitsToFloat((svdag_buffers[svdag_id].num_nodes - 1) & 0x0FFFFFFF));
	uint high_p2 = round_up_p2(max(svdag_buffers[svdag_id].voxel_width, max(svdag_buffers[svdag_id].voxel_height, svdag_buffers[svdag_id].voxel_depth)));
	while (level >= 0 && level < MAX_DEPTH) {
	    SVDAGMarchStackFrame stack_frame = stack[level];
	    vec3 stack_frame_low = stack_frame.info.xyz;
	    uint stack_frame_node_info = floatBitsToUint(stack_frame.info.w);
	    uint left_off = stack_frame_node_info >> 28;
	    SVDAGNode curr_node = svdag_buffers[svdag_id].nodes[stack_frame_node_info & 0x0FFFFFFF];
	    float diff = float(high_p2) * pow(0.5, level + 1);
	    for (uint idx = left_off; idx < 8; ++idx) {
		uint child = children_iteration_order(direction_kind, idx);
		uint offset = curr_node.child_offsets_[child];
		vec3 sub_low = stack_frame_low + subpositions(child) * diff;
		vec3 sub_high = sub_low + diff;
		aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
		bool intersects_valid_voxel = offset != SVDAG_INVALID_OFFSET && hit.front_t != -FAR_AWAY;
		if (intersects_valid_voxel) {
		    uint leaf = offset >> 31;
		    uint child_node_id = offset & 0x7FFFFFFF;
		    if (bool(leaf)) {
			vec3 obj_ray_voxel_intersect_point = obj_ray_pos + obj_ray_dir * max(hit.front_t, 0.0);
			float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_voxel_intersect_point, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
			leaf_id = child_node_id;
			reportIntersectionEXT(intersect_time, hit.k);
			return;
		    } else {
			vec4 new_this_frame = vec4(stack_frame_low, uintBitsToFloat((stack_frame_node_info & 0x0FFFFFFF) | ((idx + 1) << 28)));
			vec4 new_next_frame = vec4(sub_low, uintBitsToFloat(child_node_id & 0x0FFFFFFF));
			stack[level].info = new_this_frame;
			++level;
			stack[level].info = new_next_frame;
			++level;
			break;
		    }
		}
	    }
	    --level;
	}
    }
}
