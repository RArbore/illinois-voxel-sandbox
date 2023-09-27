#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

hitAttributeEXT uint leaf_id;

#define MAX_DEPTH 12

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

struct SVOMarchStackFrame {
    uint node_id;
    uint8_t valid_mask;
    uint8_t num_valid;
    vec3 low;
    vec3 high;
};

void main() {
    uint svo_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(1.0), obj_ray_pos, obj_ray_dir);
    if (r.front_t != -FAR_AWAY) {
	float best_intersect_time = 100000.0;
	int level = 0;
	SVOMarchStackFrame stack[MAX_DEPTH];
	stack[level].node_id = svo_buffers[svo_id].num_nodes - 1;
	stack[level].valid_mask = uint8_t(0xFF);
	stack[level].num_valid = uint8_t(0);
	stack[level].low = vec3(0.0);
	stack[level].high = vec3(1.0);
	while (level >= 0 && level < MAX_DEPTH) {
	    uint curr_node = stack[level].node_id;
	    int valid = stack[level].valid_mask & svo_buffers[svo_id].nodes[curr_node].valid_mask_;
	    for (int child = 0; child < 8; ++child) {
		int valid = valid >> (7 - child);
		valid &= 1;
		if (bool(valid)) {
		    stack[level].valid_mask ^= uint8_t(1 << (7 - child));
		    uint8_t old_num_valid = stack[level].num_valid;
		    ++stack[level].num_valid;
		    vec3 diff = stack[level].high - stack[level].low;
		    vec3 sub_low = stack[level].low + subpositions[child] * diff;
		    vec3 sub_high = sub_low + diff * 0.5;
		    aabb_intersect_result r = hit_aabb(sub_low, sub_high, obj_ray_pos, obj_ray_dir);
		    if (r.front_t != -FAR_AWAY && r.front_t < best_intersect_time) {
			int leaf = svo_buffers[svo_id].nodes[curr_node].leaf_mask_;
			leaf >>= (7 - child);
			leaf &= 1;
			uint child_node_id = svo_buffers[svo_id].nodes[curr_node].child_pointer_ + old_num_valid;
			if (bool(leaf)) {
			    vec3 obj_ray_voxel_intersect_point = obj_ray_pos + obj_ray_dir * max(r.front_t, 0.0);
			    float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_voxel_intersect_point, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
			    if (intersect_time < best_intersect_time) {
				leaf_id = child_node_id;
				best_intersect_time = intersect_time;
				reportIntersectionEXT(best_intersect_time, 0);
			    }
			} else {
			    ++level;
			    stack[level].node_id = child_node_id;
			    stack[level].valid_mask = uint8_t(0xFF);
			    stack[level].num_valid = uint8_t(0);
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
