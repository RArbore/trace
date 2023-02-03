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
#pragma shader_stage(raygen)
#extension GL_GOOGLE_include_directive : enable

#define RAY_TRACING
#include "pbr_common.glsl"

layout(location = 0) rayPayloadEXT hit_payload prd;

void main() {
    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 in_UV = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = in_UV * 2.0 - 1.0;
    
    mat4 centered_camera = camera;
    centered_camera[3][0] = 0.0;
    centered_camera[3][1] = 0.0;
    centered_camera[3][2] = 0.0;
    vec3 cam_pos = (inverse(camera) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse(perspective) * vec4(d, 0.0, 1.0)).xyz);
    
    traceRayEXT(tlas,                 // acceleration structure
		gl_RayFlagsOpaqueEXT, // rayFlags
		0xFF,                 // cullMask
		0,                    // sbtRecordOffset
		0,                    // sbtRecordStride
		0,                    // missIndex
		cam_pos,              // ray origin
		0.001,               // ray min range
		ray_dir,              // ray direction
		10000.0,              // ray max range
		0                     // payload (location = 0)
		);

    hit_payload first_hit = prd;

    uint light_id = 0;
    vec3 light_position = lights[light_id + 1].xyz;
    float light_intensity = lights[light_id + 1].w;
    vec3 light_dir = normalize(light_position - first_hit.hit_position);
    float light_dist = length(light_position - first_hit.hit_position);
    vec3 hit_position = first_hit.hit_position - ray_dir * 0.01;
    traceRayEXT(tlas,
		gl_RayFlagsOpaqueEXT,
		0xFF,
		0,
		0,
		0,
		hit_position,
		0.001,
		light_dir,
		light_dist,
		0
		);

    bool occluded = length(prd.normal) > 0.0;
    
    vec3 color = first_hit.albedo;
    if (occluded) {
	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(color * 0.05, 1.0));
    } else {
	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(color, 1.0));
    }
}
