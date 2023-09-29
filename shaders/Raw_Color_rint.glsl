#version 460
#pragma shader_stage(intersect)
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

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

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;

    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(1.0), obj_ray_pos, obj_ray_dir);

    if (r.front_t != -FAR_AWAY) {
	ivec3 volume_size = imageSize(volumes[volume_id]);
	vec3 obj_ray_intersect_point = (obj_ray_pos + obj_ray_dir * max(r.front_t, 0.0)) * volume_size;
	ivec3 obj_ray_voxel = ivec3(min(obj_ray_intersect_point, volume_size - 1));
	ivec3 obj_ray_step = ivec3(sign(obj_ray_dir));
	vec3 obj_ray_delta = abs(vec3(length(obj_ray_dir)) / obj_ray_dir);
	vec3 obj_side_dist = (sign(obj_ray_dir) * (vec3(obj_ray_voxel) - obj_ray_intersect_point) + (sign(obj_ray_dir) * 0.5) + 0.5) * obj_ray_delta;

	uint steps = 0;
	uint max_steps = uint(volume_size.x) + uint(volume_size.y) + uint(volume_size.z);
	while (steps < max_steps && all(greaterThanEqual(obj_ray_voxel, ivec3(0))) && all(lessThan(obj_ray_voxel, volume_size))) {
	    float palette = imageLoad(volumes[volume_id], obj_ray_voxel).a;
	    
	    if (palette > 0.0) {
		aabb_intersect_result r = hit_aabb(vec3(obj_ray_voxel) / volume_size, vec3(obj_ray_voxel + 1) / volume_size, obj_ray_pos, obj_ray_dir);
		vec3 obj_ray_voxel_intersect_point = obj_ray_pos + obj_ray_dir * max(r.front_t, 0.0);
		float intersect_time = length(gl_ObjectToWorldEXT * vec4(obj_ray_voxel_intersect_point, 1.0) - gl_ObjectToWorldEXT * vec4(obj_ray_pos, 1.0));
		reportIntersectionEXT(intersect_time, r.k);
		return;
	    }

	    bvec3 mask = lessThanEqual(obj_side_dist.xyz, min(obj_side_dist.yzx, obj_side_dist.zxy));
	    
	    obj_side_dist += vec3(mask) * obj_ray_delta;
	    obj_ray_voxel += ivec3(mask) * obj_ray_step;
	    ++steps;
	}
    }
}
