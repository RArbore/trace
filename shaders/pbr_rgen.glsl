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

vec3 eval_brdf(vec3 omega_in, vec3 omega_out, hit_payload hit) {
    /*vec3 c_spec = mix(vec3(0.04), hit.albedo, hit.metallicity);
    vec3 n = hit.normal;
    vec3 h = normalize(omega_in + omega_out);
    float m = hit.roughness;

    float D = exp((dot(n, h) * dot(n, h) - 1) / (m * m * dot(n, h) * dot(n, h))) / (PI * m * m * dot(n, h) * dot(n, h) * dot(n, h) * dot(n, h));
    vec3 F = c_spec + (1 - c_spec) * pow(1 - dot(omega_in, h), 5.0);
    float G = min(1.0, min(2 * dot(n, h) * dot(n, omega_out) / dot(omega_out, h), 2 * dot(n, h) * dot(n, omega_in) / dot(omega_out, h)));
    
    return D * F * G / (4 * dot(n, omega_in) * dot(n, omega_out));*/
    return vec3(1.0 / (2.0 * PI));
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
    uint light_id = 0;
    vec3 light_position = lights[light_id + 1].xyz;
    float light_intensity = lights[light_id + 1].w;
    vec3 outward_radiance = vec3(0.0);
    vec3 weight = vec3(1.0);
    
    vec3 ray_pos = (inverse(camera) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse(perspective) * vec4(d, 0.0, 1.0)).xyz);

    hit_payload hits[NUM_BOUNCES];
    float direct_light_samples[NUM_BOUNCES];
    uint hit_num;

    for (hit_num = 0; hit_num < NUM_BOUNCES; ++hit_num) {
	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, ray_dir, 10000.0, 0);
	hits[hit_num] = prd;
	if (prd.normal == vec3(0.0)) {
	    break;
	}

	vec3 light_dir = normalize(light_position - prd.hit_position);
	float light_dist = length(light_position - prd.hit_position);
	float lambert = dot(hits[hit_num].normal, -ray_dir);
	float lambert_light = dot(hits[hit_num].normal, light_dir);
	vec3 omega_in = -ray_dir;

	ray_pos = prd.hit_position + prd.flat_normal * SURFACE_OFFSET;
	ray_dir = uniform_hemisphere(slice_2_from_4(random, hit_num), prd.normal);//reflect(ray_dir, prd.normal);

	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, light_dir, light_dist, 0);
	direct_light_samples[hit_num] = light_intensity * float(prd.normal == vec3(0.0)) / (light_dist * light_dist);

	outward_radiance += hits[hit_num].albedo * direct_light_samples[hit_num] * lambert_light * weight * 2.0 * PI * eval_brdf(omega_in, light_dir, hits[hit_num]);
	weight *= lambert * 2.0 * PI * eval_brdf(omega_in, ray_dir, hits[hit_num]);
    }

    vec4 color = vec4(outward_radiance, 1.0);
    imageStore(ray_tracing_output_image, ivec2(gl_LaunchIDEXT.xy), color);
}
