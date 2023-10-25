#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

#define MAX_DEPTH 32

struct aabb_intersect_result {
    float front_t;
    float back_t;
    uint k;
};

aabb_intersect_result hit_aabb(const vec3 minimum, const vec3 maximum, const vec3 origin, const vec3 direction) {
    aabb_intersect_result r;
    vec3 invDir = 1.0 / direction;
    vec3 tbot = invDir * (minimum - origin);
    vec3 ttop = invDir * (maximum - origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    float t0 = max(tmin.x, max(tmin.y, tmin.z));
    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    r.front_t = t1 > max(t0, 0.0) ? t0 : -FAR_AWAY;
    r.back_t = t1 > max(t0, 0.0) ? t1 : -FAR_AWAY;
    if (t0 == tmin.x) {
	r.k = tbot.x > ttop.x ? 1 : 0;
    } else if (t0 == tmin.y) {
	r.k = tbot.y > ttop.y ? 3 : 2;
    } else if (t0 == tmin.z) {
	r.k = tbot.z > ttop.z ? 5 : 4;
    }
    return r;
}

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
				     vec3(0.5, 0.0, 0.0),
				     vec3(0.0, 0.5, 0.0),
				     vec3(0.5, 0.5, 0.0),
				     vec3(0.0, 0.0, 0.5),
				     vec3(0.5, 0.0, 0.5),
				     vec3(0.0, 0.5, 0.5),
				     vec3(0.5, 0.5, 0.5)
				     );
*/

vec3 subpositions(uint child) {
    return vec3(
		bool(child % 2) ? 0.5 : 0.0,
		bool((child / 2) % 2) ? 0.5 : 0.0,
		bool((child / 4) % 2) ? 0.5 : 0.0
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
    uint node_id;
    uint left_off;
    vec3 low;
    vec3 high;
};

void main() {
    uint svdag_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);

    int direction_kind = int(obj_ray_dir.x < 0.0) + 2 * int(obj_ray_dir.y < 0.0) + 4 * int(obj_ray_dir.z < 0.0);

    vec3 first_low = vec3(0.0);
    vec3 first_high = vec3(svdag_buffers[svdag_id].voxel_width, svdag_buffers[svdag_id].voxel_height, svdag_buffers[svdag_id].voxel_depth);
    aabb_intersect_result bounding = hit_aabb(first_low, first_high, obj_ray_pos, obj_ray_dir);
    if (bounding.front_t != -FAR_AWAY) {
	int level = 0;
	SVDAGMarchStackFrame stack[MAX_DEPTH];
	stack[level].node_id = svdag_buffers[svdag_id].num_nodes - 1;
	stack[level].left_off = 0;
	stack[level].low = first_low;
	uint high_p2 = round_up_p2(max(svdag_buffers[svdag_id].voxel_width, max(svdag_buffers[svdag_id].voxel_height, svdag_buffers[svdag_id].voxel_depth)));
	stack[level].high = vec3(high_p2);
	while (level >= 0 && level < MAX_DEPTH) {
	    SVDAGMarchStackFrame stack_frame = stack[level];
	    SVDAGNode curr_node = svdag_buffers[svdag_id].nodes[stack_frame.node_id];
	    for (uint idx = stack_frame.left_off; idx < 8; ++idx) {
		uint child = children_iteration_order(direction_kind, idx);
		uint offset = curr_node.child_offsets_[child];
		if (offset != SVDAG_INVALID_OFFSET) {
		    vec3 diff = stack_frame.high - stack_frame.low;
		    vec3 sub_low = stack_frame.low + subpositions(child) * diff;
		    vec3 sub_high = sub_low + diff * 0.5;
		    aabb_intersect_result hit = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
		    if (hit.front_t != -FAR_AWAY) {
			uint leaf = offset >> 31;
			uint child_node_id = offset & 0x7FFFFFFF;
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
