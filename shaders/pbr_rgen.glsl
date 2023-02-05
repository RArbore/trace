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

float eval_brdf(vec3 omega_in, vec3 omega_out, float roughness, float metallicity) {
    return 1.0;
}

void main() {
    const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 in_UV = pixel_center / vec2(gl_LaunchSizeEXT.xy);
    const vec2 d = in_UV * 2.0 - 1.0;
    const uvec2 blue_noise_size = imageSize(blue_noise_image);
    const uvec2 blue_noise_coords = (gl_LaunchIDEXT.xy/* + ivec2(hash(seed), hash(seed * 3))*/) % blue_noise_size;
    const float random = imageLoad(blue_noise_image,  ivec2(blue_noise_coords)).x;
    mat4 centered_camera = camera;
    centered_camera[3][0] = 0.0;
    centered_camera[3][1] = 0.0;
    centered_camera[3][2] = 0.0;

    uint num_lights = floatBitsToUint(lights[0].x);
    uint light_id = 0;//uint(round(random)) % num_lights;
    vec3 light_position = lights[light_id + 1].xyz;
    float light_intensity = lights[light_id + 1].w;
    vec3 outward_radiance = vec3(0.0);
    
    vec3 ray_pos = (inverse(camera) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse(perspective) * vec4(d, 0.0, 1.0)).xyz);

    hit_payload hits[NUM_BOUNCES];
    float direct_light_samples[NUM_BOUNCES];
    uint hit_num;

    for (hit_num = 0; hit_num < NUM_BOUNCES; ++hit_num) {
	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, ray_dir, 10000.0, 0);
	hits[hit_num] = prd;
	if (prd.normal == vec3(0.0)) {
	    ++hit_num;
	    break;
	}

	vec3 light_dir = normalize(light_position - prd.hit_position);
	float light_dist = length(light_position - prd.hit_position);
	float lambert = clamp(dot(prd.normal, light_dir), 0.0, 1.0);
	float brdf = eval_brdf(-ray_dir, light_dir, prd.roughness, prd.metallicity);
	float bsdf = brdf * lambert;

	ray_pos = prd.hit_position + prd.flat_normal * SURFACE_OFFSET;
	ray_dir = reflect(ray_dir, prd.normal);

	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, light_dir, light_dist, 0);
	direct_light_samples[hit_num] = light_intensity * bsdf * float(prd.normal == vec3(0.0)) / (light_dist * light_dist);

	outward_radiance += hits[hit_num].albedo * direct_light_samples[hit_num];
    }

    vec4 color = vec4(outward_radiance, 1.0);
    imageStore(ray_tracing_output_image, ivec2(gl_LaunchIDEXT.xy), color);
}
