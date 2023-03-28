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
#pragma shader_stage(closest)
#extension GL_GOOGLE_include_directive : enable

#define RAY_TRACING
#include "common.glsl"

layout(location = 0) rayPayloadInEXT hit_payload prd;

void main() {
    vec4 light = lights[gl_GeometryIndexEXT + 1];
    vec3 hit_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(hit_pos - light.xyz);

    prd.albedo = vec3(1.0);
    prd.normal = normal;
    prd.flat_normal = normal;
    prd.roughness = 1.0;
    prd.metallicity = 0.0;
    prd.hit_position = hit_pos;
    prd.direct_emittance = light.w;
    prd.model_id = 0xFFFFFFFE;
}
