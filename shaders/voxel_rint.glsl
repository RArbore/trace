/*
 * This file is part of trace.
 * trace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * trace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with trace. If not, see <https://www.gnu.org/licenses/>.
 */


#version 460
#pragma shader_stage(intersect)
#extension GL_GOOGLE_include_directive : enable

#define RAY_TRACING
#include "common.glsl"

struct aabb_intersect_result {
    float t;
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
    r.t = t1 > max(t0, 0.0) ? t0 : -FAR_AWAY;
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
    if (r.t != -FAR_AWAY) {
	ivec3 volume_size = imageSize(volumes[volume_id]);
	vec3 obj_ray_intersect_point = (obj_ray_pos + obj_ray_dir * max(r.t, 0.0)) * volume_size;
	ivec3 obj_ray_voxel = ivec3(min(obj_ray_intersect_point, volume_size - 1));
	ivec3 obj_ray_step = ivec3(sign(obj_ray_dir));
	vec3 obj_ray_delta = abs(vec3(length(obj_ray_dir)) / obj_ray_dir);
	vec3 obj_side_dist = (sign(obj_ray_dir) * (vec3(obj_ray_voxel) - obj_ray_intersect_point) + (sign(obj_ray_dir) * 0.5) + 0.5) * obj_ray_delta;

	uint steps = 0;
	uint max_steps = uint(volume_size.x) + uint(volume_size.y) + uint(volume_size.z);
	while (steps < max_steps && all(greaterThanEqual(obj_ray_voxel, ivec3(0))) && all(lessThan(obj_ray_voxel, volume_size))) {
	    float palette = imageLoad(volumes[volume_id], obj_ray_voxel).r;
	    
	    if (palette > 0.0) {
		r = hit_aabb(vec3(obj_ray_voxel) / volume_size, vec3(obj_ray_voxel + 1) / volume_size, obj_ray_pos, obj_ray_dir);
		reportIntersectionEXT(r.t, r.k);
		return;
	    }

	    bvec3 mask = lessThanEqual(obj_side_dist.xyz, min(obj_side_dist.yzx, obj_side_dist.zxy));
	    
	    obj_side_dist += vec3(mask) * obj_ray_delta;
	    obj_ray_voxel += ivec3(mask) * obj_ray_step;
	    ++steps;
	}
    }
}
