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
    vec4 light = lights[gl_PrimitiveID];

    vec3 d = gl_WorldRayDirectionEXT;
    vec3 p_r = gl_WorldRayOriginEXT;
    vec3 p_s = light.xyz;
    float r = LIGHT_RADIUS;
    
    float a = dot(d, d);
    float b = 2.0 * dot(d, p_r - p_s);
    float c = dot(p_s, p_s) + dot(p_r, p_r) - 2.0 * dot(p_s, p_r) - r * r;
    
    float test = b * b - 4.0 * a * c;
    if (test >= 0.0) {
	float t = (b + sqrt(test)) / (-2.0 * a);
	reportIntersectionEXT(t, 0);
    }
}
