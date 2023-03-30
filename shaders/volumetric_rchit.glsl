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
    vec3 obj_ray_pos = gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0);
    vec3 obj_ray_dir = gl_WorldToObjectEXT * vec4(gl_WorldRayDirectionEXT, 0.0);
    aabb_intersect_result r = hit_aabb(vec3(0.0), vec3(1.0), obj_ray_pos, obj_ray_dir);

    vec3 world_ray_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * r.t;
    vec3 world_ray_back_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * (r.back_t + SURFACE_OFFSET);
    vec3 normal = normalize((voxel_normals[gl_HitKindEXT] * gl_WorldToObjectEXT).xyz);

    prd.albedo = vec3(0.5);
    prd.normal = normal;
    prd.flat_normal = normal;
    prd.roughness = 1.0;
    prd.metallicity = 0.0;
    prd.hit_position = world_ray_back_pos;
    prd.direct_emittance = 0.0;
    prd.model_kind = KIND_VOLUMETRIC;
    prd.model_id = gl_InstanceCustomIndexEXT;
    prd.volumetric_weight = 0.2;
}
