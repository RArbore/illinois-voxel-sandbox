#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

#define MAX_DEPTH 32

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

const vec3 subpositions[8] = vec3[8](
				     vec3(0.0, 0.0, 0.0),
				     vec3(0.5, 0.0, 0.0),
				     vec3(0.0, 0.5, 0.0),
				     vec3(0.5, 0.5, 0.0),
				     vec3(0.0, 0.0, 0.5),
				     vec3(0.5, 0.0, 0.5),
				     vec3(0.0, 0.5, 0.5),
				     vec3(0.5, 0.5, 0.5)
				     );

const uint children_iteration_order[8][8] = uint[8][8](
						       uint[8](0, 1, 2, 3, 4, 5, 6, 7),
						       uint[8](1, 0, 3, 2, 5, 4, 7, 6),
						       uint[8](2, 3, 0, 1, 6, 7, 4, 5),
						       uint[8](3, 2, 1, 0, 7, 6, 5, 4),
						       uint[8](4, 5, 6, 7, 0, 1, 2, 3),
						       uint[8](5, 4, 7, 6, 1, 0, 3, 2),
						       uint[8](6, 7, 4, 5, 2, 3, 0, 1),
						       uint[8](7, 6, 5, 4, 3, 2, 1, 0)
						       );

struct SVOMarchStackFrame {
    uint node_id;
    uint left_off;
    vec3 low;
    vec3 high;
};

void main() {
    uint svo_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);

    vec3 first_low = vec3(0.0);
    vec3 first_high = vec3(svo_buffers[svo_id].voxel_width, svo_buffers[svo_id].voxel_height, svo_buffers[svo_id].voxel_depth);
    aabb_intersect_result bounding = hit_aabb(first_low, first_high, obj_ray_pos, obj_ray_dir);
    if (bounding.front_t != -FAR_AWAY) {
	int level = 0;
	SVOMarchStackFrame stack[MAX_DEPTH];
	stack[level].node_id = svo_buffers[svo_id].num_nodes - 1;
	stack[level].left_off = 0;
	stack[level].low = first_low;
	uint high_p2 = round_up_p2(max(svo_buffers[svo_id].voxel_width, max(svo_buffers[svo_id].voxel_height, svo_buffers[svo_id].voxel_depth)));
	stack[level].high = vec3(high_p2);
	while (level >= 0 && level < MAX_DEPTH) {
	    uint curr_node = stack[level].node_id;
	    int valid_mask = svo_buffers[svo_id].nodes[curr_node].valid_mask_;
	    for (uint idx = stack[level].left_off; idx < 8; ++idx) {
		int child = int(children_iteration_order[direction_kind][idx]);
		int valid = valid_mask >> (7 - child);
		valid &= 1;
		if (bool(valid)) {
		    vec3 diff = stack[level].high - stack[level].low;
		    vec3 sub_low = stack[level].low + subpositions[child] * diff;
		    vec3 sub_high = sub_low + diff * 0.5;
		    aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
		    if (hit.front_t != -FAR_AWAY) {
			int leaf = svo_buffers[svo_id].nodes[curr_node].leaf_mask_;
			leaf >>= (7 - child);
			leaf &= 1;
			uint8_t num_valid = uint8_t(0);
			for (int idx = 0; idx < child; ++idx) {
			    if (bool((valid_mask >> (7 - idx)) & 1)) {
				++num_valid;
			    }
			}
			uint child_node_id = curr_node - svo_buffers[svo_id].nodes[curr_node].child_offset_ + num_valid;
			if (bool(leaf)) {
			    vec3 obj_ray_voxel_intersect_point = obj_ray_pos + obj_ray_dir * max(hit.front_t, 0.0);
			    float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_voxel_intersect_point, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
			    leaf_id = child_node_id;
			    reportIntersectionEXT(intersect_time, hit.k);
			    return;
			} else {
			    stack[level].left_off = idx + 1;
			    ++level;
			    stack[level].node_id = child_node_id;
			    stack[level].left_off = 0;
			    stack[level].low = sub_low;
			    stack[level].high = sub_high;
			    ++level;
			    break;
			}
		    }
		}
	    }
	    --level;
	}
    }
}
