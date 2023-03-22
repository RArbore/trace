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

const vec3 voxel_normals[6] = vec3[6](
				      vec3(-1.0, 0.0, 0.0),
				      vec3(1.0, 0.0, 0.0),
				      vec3(0.0, -1.0, 0.0),
				      vec3(0.0, 1.0, 0.0),
				      vec3(0.0, 0.0, -1.0),
				      vec3(0.0, 0.0, 1.0)
				      );
const vec3 custom_colors[3] = vec3[3](
				      vec3(1.0, 0.0, 0.0),
				      vec3(0.0, 1.0, 0.0),
				      vec3(0.0, 0.0, 1.0)
				      );


void main() {
    vec3 world_ray_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 world_obj_pos = gl_ObjectToWorldEXT * vec4(0.0, 0.0, 0.0, 1.0);
    vec3 normal = normalize((voxel_normals[gl_HitKindEXT] * gl_WorldToObjectEXT).xyz);

    vec3 voxel_sample_pos = gl_WorldToObjectEXT * vec4(world_ray_pos, 1.0);
    ivec3 volume_load_pos = ivec3(voxel_sample_pos * vec3(imageSize(volumes[gl_InstanceCustomIndexEXT])) - 0.5 * voxel_normals[gl_HitKindEXT]);
    uint palette = uint(256.0 * imageLoad(volumes[gl_InstanceCustomIndexEXT], volume_load_pos).r);
    
    uint palette_lookup = p[gl_InstanceCustomIndexEXT * 256 + palette];
    uint palette_r = palette_lookup >> 24;
    uint palette_g = (palette_lookup >> 16) & 0xFF;
    uint palette_b = (palette_lookup >> 8) & 0xFF;
    
    prd.albedo = vec3(palette_r, palette_g, palette_b) / 255.0;
    //prd.albedo = custom_colors[palette];
    prd.normal = normal;
    prd.flat_normal = normal;
    prd.roughness = 1.0;
    prd.metallicity = 0.0;
    prd.hit_position = world_ray_pos;
    prd.direct_emittance = 0.0;
    prd.model_id = gl_InstanceCustomIndexEXT;
}
