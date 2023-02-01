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
#extension GL_EXT_ray_tracing : require

struct hit_payload {
    vec3 albedo;
    vec3 normal;
    float roughness;
    float metallicity;
};

layout(location = 0) rayPayloadEXT hit_payload prd;

layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 1, binding = 1, rgba8) uniform writeonly image2D image;

layout (push_constant) uniform PushConstants {
    mat4 perspective;
    mat4 camera;
};

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
		0.001,                // ray min range
		ray_dir,              // ray direction
		10000.0,              // ray max range
		0                     // payload (location = 0)
		);
    
    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(vec3(prd.normal * 0.5 + 0.5), 1.0));
}
