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
#pragma shader_stage(miss)
#extension GL_GOOGLE_include_directive : enable

#define RAY_TRACING
#include "pbr_common.glsl"

layout(location = 0) rayPayloadInEXT hit_payload prd;

void main() {
    prd.albedo = vec3(0.0);
    prd.normal = vec3(0.0);
    prd.flat_normal = vec3(0.0);
    prd.roughness = 0.0;
    prd.metallicity = 0.0;
    prd.hit_position = vec3(0.0);
}
