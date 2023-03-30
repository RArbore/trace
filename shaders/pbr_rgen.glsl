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

float atan2(in float y, in float x) {
    bool s = (abs(x) > abs(y));
    return mix(PI / 2.0 - atan(x,y), atan(y,x), s);
}

vec2 slice_2_from_4(vec4 random, uint num) {
    uint slice = (num + current_frame) % 4;
    return random.xy * float(slice == 0) + random.yz * float(slice == 1) + random.zw * float(slice == 2) + random.wx * float(slice == 3);
}

float normal_distribution(float cos_theta, float alpha) {
    float alpha_2 = alpha * alpha;
    float cos_theta_2 = cos_theta * cos_theta;
    
    float nominator = alpha_2;
    float denominator = (cos_theta_2 * (alpha_2 - 1.0) + 1.0);
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

mat3 get_arbitrary_hemisphere_orientation_matrix(vec3 direction) {
    vec3 random_direction = direction.x != 0.0 || direction.y != 0.0 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 x = normalize(cross(direction, random_direction));
    vec3 y = cross(x, direction);
    return mat3(x, y, direction);
}

ray_sample uniform_weighted_hemisphere(vec2 random, vec3 direction) {
    ray_sample ret;
    float theta = acos(random.x);
    float phi = 2.0 * PI * random.y;
    vec3 up_hemisphere = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
    ret.drawn_sample = get_arbitrary_hemisphere_orientation_matrix(direction) * up_hemisphere;
    ret.drawn_weight = 1.0 / PI;
    return ret;
}

vec3 uniform_weighted_hemisphere_simple(vec2 random, vec3 direction) {
    float theta = acos(random.x);
    float phi = 2.0 * PI * random.y;
    return get_arbitrary_hemisphere_orientation_matrix(direction) * vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
}

vec3 BRDF(vec3 omega_in, vec3 omega_out, hit_payload hit) {
    vec3 F0_dieletric = vec3(0.04); 
    vec3 F0 = mix(F0_dieletric, hit.albedo, hit.metallicity);
    float alpha = hit.roughness * hit.roughness;
    float k = (hit.roughness + 1.0) * (hit.roughness + 1.0) / 8.0;
    vec3 halfway_dir = normalize(omega_in + omega_out);

    float D = normal_distribution(max(dot(hit.normal, halfway_dir), 0.0), alpha);
    float G = geometry_smith(hit.normal, omega_in, omega_out, k);
    vec3 F = fresnel_schlick(clamp(dot(halfway_dir, omega_in), 0.0, 1.0), F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(hit.normal, omega_in), 0.0) * max(dot(hit.normal, omega_out), 0.0)  + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - hit.metallicity);
    return kD * hit.albedo / PI + specular;
}

ray_sample sample_light_sources(vec2 random, vec3 origin, vec3 normal) {
    vec4 light = lights[1]; // "random"
    if (dot(normal, light.xyz - origin) < 0.0) {
	ray_sample samp;
	samp.drawn_sample = vec3(0.0);
	samp.drawn_weight = 0.0;
	return samp;
    }
    vec3 hemisphere_point = uniform_weighted_hemisphere_simple(random, normal);
    vec3 sample_point = hemisphere_point * LIGHT_RADIUS + light.xyz;
    vec3 dist = sample_point - origin;
    float dist2 = dot(dist, dist);
    ray_sample samp;
    samp.drawn_sample = dist2 > 0.01 ? normalize(dist) : vec3(0.0);
    samp.drawn_weight = dist2 > 0.01 ? 1.0 / dist2 : 0.0;
    return samp;
}

void main() {
    const uvec2 blue_noise_size = imageSize(blue_noise_image);
    const uvec2 blue_noise_coords = (gl_LaunchIDEXT.xy + ivec2(hash(current_frame), hash(3 * current_frame))) % blue_noise_size;
    const vec4 random = imageLoad(blue_noise_image,  ivec2(blue_noise_coords));

    vec3 outward_radiance = vec3(0.0);
    vec3 weight = vec3(1.0);
    
    vec2 device_coord = pixel_coord_to_device_coord(gl_LaunchIDEXT.xy);
    vec3 ray_pos = camera_position;
    vec3 ray_dir = normalize((centered_inverse_camera * inverse_jittered_perspective * vec4(device_coord, 0.0, 1.0)).xyz);

    bool found_first_hit = false;
    hit_payload first_hit;
    for (uint hit_num = 0; hit_num < NUM_BOUNCES && any(greaterThan(weight, vec3(WEIGHT_CUTOFF))); ++hit_num) {
	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, ray_dir, FAR_AWAY, 0);
	hit_payload indirect_prd = prd;

	if (indirect_prd.model_kind != KIND_LIGHT || hit_num == 0) {
	    outward_radiance += indirect_prd.direct_emittance * weight;
	}
	if (!found_first_hit) {
	    first_hit = indirect_prd;
	    indirect_prd.albedo = vec3(1.0);
	    outward_radiance *= 2.0;
	    found_first_hit = true;
	}
	if (indirect_prd.model_kind == KIND_MISS) {
	    break;
	}

	if (indirect_prd.model_kind != KIND_VOLUMETRIC) {
	    ray_pos = indirect_prd.hit_position + indirect_prd.flat_normal * SURFACE_OFFSET;
	    ray_sample direct_sample = sample_light_sources(slice_2_from_4(random, hit_num), ray_pos, indirect_prd.normal);
	    if (direct_sample.drawn_weight > 0.0) {
		traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, ray_pos, 0.001, direct_sample.drawn_sample, FAR_AWAY, 0);
		hit_payload direct_prd = prd;
		
		vec3 direct_brdf = BRDF(-ray_dir, direct_sample.drawn_sample, indirect_prd);
		float direct_lambert = dot(direct_sample.drawn_sample, indirect_prd.normal);
		outward_radiance += direct_prd.direct_emittance * weight * direct_lambert * direct_brdf * direct_sample.drawn_weight;
	    }
	    
	    float indirect_lambert = dot(indirect_prd.normal, -ray_dir);
	    vec3 omega_in = -ray_dir;
	    ray_sample indirect_sample = uniform_weighted_hemisphere(slice_2_from_4(random, hit_num + 2), indirect_prd.normal);
	    ray_dir = indirect_sample.drawn_sample;
	    weight *= indirect_lambert * BRDF(omega_in, ray_dir, indirect_prd) / indirect_sample.drawn_weight;
	} else {
	    ray_pos = indirect_prd.hit_position;
	    weight *= indirect_prd.volumetric_weight;
	}
    }
    if (!found_first_hit) {
	first_hit = create_miss(ray_pos, ray_dir);
    }

    vec3 hit_position = first_hit.hit_position;
    vec4 new_device = perspective * camera * vec4(hit_position, 1.0);
    vec4 old_device = perspective * last_frame_camera * vec4(hit_position, 1.0);
    vec2 pixel_velocity = device_coord_to_pixel_coord(new_device.xy / new_device.w) - device_coord_to_pixel_coord(old_device.xy / old_device.w);

    outward_radiance /= 2.0;
    float lum = luminance(outward_radiance);
    if (current_frame % 2 == 0) {
	imageStore(ray_trace1_albedo_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit.albedo, float(first_hit.model_id % 256) / 255.0));
	imageStore(ray_trace1_lighting1_image, ivec2(gl_LaunchIDEXT.xy), vec4(outward_radiance, 1.0));
	imageStore(ray_trace1_position_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit.hit_position, 1.0));
	imageStore(ray_trace1_normal_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit.normal * 0.5 + 0.5, 1.0));
	imageStore(ray_trace1_history1_image, ivec2(gl_LaunchIDEXT.xy), vec4(lum, lum * lum, 0.0, 1.0));
    } else {
	imageStore(ray_trace2_albedo_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit.albedo, float(first_hit.model_id % 256) / 255.0));
	imageStore(ray_trace2_lighting1_image, ivec2(gl_LaunchIDEXT.xy), vec4(outward_radiance, 1.0));
	imageStore(ray_trace2_position_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit.hit_position, 1.0));
	imageStore(ray_trace2_normal_image, ivec2(gl_LaunchIDEXT.xy), vec4(first_hit.normal * 0.5 + 0.5, 1.0));
	imageStore(ray_trace2_history1_image, ivec2(gl_LaunchIDEXT.xy), vec4(lum, lum * lum, 0.0, 1.0));
    }
    imageStore(motion_vector_image, ivec2(gl_LaunchIDEXT.xy), vec4(pixel_velocity, 0.0, 1.0));
}
