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
    vec3 world_ray_pos = gl_WorldRayOriginEXT;
    vec3 world_obj_pos = gl_ObjectToWorldEXT * vec4(0.0, 0.0, 0.0, 1.0);
    float radius = 1.0;

    vec3 oc = gl_WorldRayOriginEXT - world_obj_pos;
    float a = dot(gl_WorldRayDirectionEXT, gl_WorldRayDirectionEXT);
    float b = 2.0 * dot(oc, gl_WorldRayDirectionEXT);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    if(discriminant >= 0) {
	float t = (-b - sqrt(discriminant)) / (2.0 * a);
	reportIntersectionEXT(t, 0);
    }
}
