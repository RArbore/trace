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

void main() {
    uint volume_id = gl_InstanceCustomIndexEXT;
    
    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);

    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(1.0), obj_ray_pos, obj_ray_dir);
    if (r.t != -FAR_AWAY) {
	reportIntersectionEXT(r.t, r.k);
    }
}
