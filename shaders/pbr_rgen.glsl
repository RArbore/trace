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

float normal_distribution(vec3 normal, vec3 halfway, float alpha) {
    float alpha_2 = alpha * alpha;
    float normal_dot_halfway = max(dot(normal, halfway), 0.0);
    float normal_dot_halfway_2 = normal_dot_halfway * normal_dot_halfway;
    
    float nominator = alpha_2;
    float denominator = (normal_dot_halfway_2 * (alpha_2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    
    return nominator / denominator;
}

float geometry_schlick(float normal_dot_view, float k) {
    float nominator = normal_dot_view;
    float denominator = normal_dot_view * (1.0 - k) + k;
	
    return nominator / denominator;
}
  
float geometry_smith(vec3 normal, vec3 view, vec3 light, float k) {
    float normal_dot_view = max(dot(normal, view), 0.0);
    float normal_dot_light = max(dot(normal, light), 0.0);
    float ggx1 = geometry_schlick(normal_dot_view, k);
    float ggx2 = geometry_schlick(normal_dot_light, k);
	
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 BRDF(vec3 omega_in, vec3 omega_out, hit_payload hit) {
    vec3 F0_dieletric = vec3(0.04); 
    vec3 F0 = mix(F0_dieletric, hit.albedo, hit.metallicity);
    float alpha = hit.roughness * hit.roughness;
    float k = (hit.roughness + 1.0) * (hit.roughness + 1.0) / 8.0;
    vec3 halfway_dir = normalize(omega_in + omega_out);

    float D = normal_distribution(hit.normal, halfway_dir, alpha);
    float G = geometry_smith(hit.normal, omega_in, omega_out, k);
    vec3 F = fresnel_schlick(clamp(dot(halfway_dir, omega_in), 0.0, 1.0), F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(hit.normal, omega_in), 0.0) * max(dot(hit.normal, omega_out), 0.0)  + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - hit.metallicity);
    return kD * hit.albedo / PI + specular;
}

vec2 pixel_coord_to_device_coord(ivec2 pixel_coord) {
    vec2 pixel_center = vec2(pixel_coord) + taa_jitter;
    vec2 in_UV = pixel_center / vec2(imageSize(ray_tracing_albedo_image));
    return in_UV * 2.0 - 1.0;
}

void main() {
    const uvec2 blue_noise_size = imageSize(blue_noise_image);
    const uvec2 blue_noise_coords = (gl_LaunchIDEXT.xy + ivec2(hash(seed), hash(3 * seed))) % blue_noise_size;
    const vec4 random = imageLoad(blue_noise_image,  ivec2(blue_noise_coords));

    uint num_lights = floatBitsToUint(lights[0].x);
    vec3 outward_radiance = vec3(0.0);
    vec3 weight = vec3(1.0);
    
    vec2 device_coord = pixel_coord_to_device_coord(ivec2(gl_LaunchIDEXT.xy));
    vec3 ray_pos = camera_position;
    vec3 ray_dir = normalize((inverse_centered_camera * inverse_perspective * vec4(device_coord, 0.0, 1.0)).xyz);

    hit_payload hits[NUM_BOUNCES];
    vec3 first_hit_albedo = vec3(1.0);
    uint hit_num;

    for (hit_num = 0; hit_num < NUM_BOUNCES; ++hit_num) {
	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, ray_dir, FAR_AWAY, 0);
	if (hit_num == 0) {
	    first_hit_albedo = prd.albedo;
	    prd.albedo = vec3(1.0);
	}
	hits[hit_num] = prd;
	outward_radiance += hits[hit_num].direct_emittance * weight;

	if (prd.normal == vec3(0.0)) {
	    break;
	} else {
	    float lambert = dot(hits[hit_num].normal, -ray_dir);
	    vec3 omega_in = -ray_dir;
	    ray_pos = prd.hit_position + prd.flat_normal * SURFACE_OFFSET;
	    ray_dir = uniform_hemisphere(slice_2_from_4(random, hit_num), prd.normal);
	    weight *= lambert * BRDF(omega_in, ray_dir, hits[hit_num]);
	}
    }

    imageStore(ray_tracing_albedo_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit_albedo, 1.0));
    imageStore(ray_tracing_lighting_image, ivec2(gl_LaunchIDEXT.xy), vec4(outward_radiance, 1.0));
    imageStore(ray_tracing_position_image, ivec2(gl_LaunchIDEXT.xy), vec4(hits[0].hit_position, 1.0));
    imageStore(ray_tracing_normal_image, ivec2(gl_LaunchIDEXT.xy), vec4(hits[0].normal * 0.5 + 0.5, 1.0));
}
