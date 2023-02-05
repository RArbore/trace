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
#include "common.glsl"

layout(location = 0) rayPayloadEXT hit_payload prd;

vec3 uniform_hemisphere(vec2 random, vec3 direction) {
    float theta = 2.0 * PI * random.x;
    float cos_phi = 1.0 - 2.0 * random.y;
    float sin_theta = sin(theta);
    float sin_phi = sqrt(1 - cos_phi * cos_phi);
    vec3 sphere = vec3(sin_theta * sin_phi, sin_theta * cos_phi, sin_phi);
    return dot(sphere, direction) >= 0.0 ? sphere : -sphere;
}

vec2 slice_2_from_4(vec4 random, uint num) {
    uint slice = (num + seed) % 4;
    return random.xy * float(slice == 0) + random.yz * float(slice == 1) + random.zw * float(slice == 2) + random.wx * float(slice == 3);
}

void main() {
    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 in_UV = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    const vec2 d = in_UV * 2.0 - 1.0;
    const uvec2 blue_noise_size = imageSize(blue_noise_image);
    const uvec2 blue_noise_coords = (gl_LaunchIDEXT.xy + ivec2(hash(seed), hash(3 * seed))) % blue_noise_size;
    const vec4 random = imageLoad(blue_noise_image,  ivec2(blue_noise_coords));

    mat4 centered_camera = camera;
    centered_camera[3][0] = 0.0;
    centered_camera[3][1] = 0.0;
    centered_camera[3][2] = 0.0;

    uint num_lights = floatBitsToUint(lights[0].x);
    vec3 outward_radiance = vec3(0.0);
    vec3 weight = vec3(1.0);
    
    vec3 ray_pos = (inverse(camera) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse(perspective) * vec4(d, 0.0, 1.0)).xyz);

    hit_payload hits[NUM_BOUNCES];
    uint hit_num;

    for (hit_num = 0; hit_num < NUM_BOUNCES; ++hit_num) {
	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, ray_dir, 10000.0, 0);
	hits[hit_num] = prd;
	outward_radiance += hits[hit_num].albedo * hits[hit_num].direct_emittance * weight;

	if (prd.normal == vec3(0.0)) {
	    break;
	} else {
	    float lambert = dot(hits[hit_num].normal, -ray_dir);
	    vec3 omega_in = -ray_dir;
	    ray_pos = prd.hit_position + prd.flat_normal * SURFACE_OFFSET;
	    ray_dir = uniform_hemisphere(slice_2_from_4(random, hit_num), prd.normal);
	    weight *= hits[hit_num].albedo * lambert;// * 2.0 * PI * BRDF(omega_in, ray_dir, hits[hit_num]);
	}
    }

    vec4 color = vec4(outward_radiance, 1.0);
    imageStore(ray_tracing_output_image, ivec2(gl_LaunchIDEXT.xy), color);
}
