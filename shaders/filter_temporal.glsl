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
#pragma shader_stage(compute)
#extension GL_GOOGLE_include_directive : enable

layout (local_size_x = 32, local_size_y = 32) in;

#include "common.glsl"

void main() {
    vec2 pixel_coord = vec2(gl_GlobalInvocationID.xy) - 0.5;
    vec2 texture_size = textureSize(motion_vector_texture, 0);
    if (pixel_coord.x >= texture_size.x || pixel_coord.y >= texture_size.y) {
	return;
    }
    vec2 fragment_UV = pixel_coord / texture_size;
    vec2 motion_vector = texture(motion_vector_texture, fragment_UV).xy;
    
    vec2 reprojected_pixel_coord = pixel_coord - motion_vector;
    pixel_sample new_sample = get_new_sample(pixel_coord);
    pixel_sample reprojected_sample = get_old_sample(reprojected_pixel_coord, 0);

    bool blend =
	new_sample.model_id == reprojected_sample.model_id && 
	dot(new_sample.normal, reprojected_sample.normal) > 0.8 &&
	length(new_sample.position - reprojected_sample.position) < 0.5 &&
	length(new_sample.position) < FAR_AWAY * 0.5 &&
	length(new_sample.albedo - reprojected_sample.albedo) < 0.1 &&
	length(reprojected_sample.position) < FAR_AWAY * 0.5 &&
	reprojected_pixel_coord.x >= 0 &&
	reprojected_pixel_coord.y >= 0 &&
	reprojected_pixel_coord.x < texture_size.x &&
	reprojected_pixel_coord.y < texture_size.y;
    vec3 blended_lighting = mix(reprojected_sample.lighting, new_sample.lighting, alpha_temporal);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blend ? blended_lighting : new_sample.lighting;

    float blended_luminance_moment1 = mix(reprojected_sample.luminance_moment1, new_sample.luminance_moment1, alpha_temporal);
    float blended_luminance_moment2 = mix(reprojected_sample.luminance_moment2, new_sample.luminance_moment2, alpha_temporal);
    blended_sample.luminance_moment1 = blend ? blended_luminance_moment1 : new_sample.luminance_moment1;
    blended_sample.luminance_moment2 = blend ? blended_luminance_moment2 : new_sample.luminance_moment2;
    blended_sample.history_length = blend ? reprojected_sample.history_length + 1.0 : 1.0;
    if (blended_sample.history_length >= 4.0) {
	blended_sample.variance = blended_sample.luminance_moment2 - blended_sample.luminance_moment1 * blended_sample.luminance_moment1;
    } else {
	float total_luminance_moment1 = 0.0;
	float total_luminance_moment2 = 0.0;
	float total_weight = 0.0;
	for (int i = -1; i <= 1; ++i) {
	    for (int j = -1; j <= 1; ++j) {
		ivec2 offset = ivec2(i, j);
		vec2 sample_pixel_coord = pixel_coord + offset;
		if (sample_pixel_coord.x >= 0 && sample_pixel_coord.x >= 0 && sample_pixel_coord.x < texture_size.x && sample_pixel_coord.y < texture_size.y) {
		    pixel_sample spatial_sample = get_new_sample(sample_pixel_coord);
		    
		    vec3 normal_dist = new_sample.normal - spatial_sample.normal;
		    float normal_dist2 = dot(normal_dist, normal_dist);
		    float normal_weight = min(exp(-normal_dist2 / sigma_normal), 1.0);
		    
		    vec3 position_dist = new_sample.position - spatial_sample.position;
		    float position_dist2 = dot(position_dist, position_dist);
		    float position_weight = min(exp(-position_dist2 / sigma_position), 1.0);

		    float weight = normal_weight * position_weight;
		    total_luminance_moment1 += spatial_sample.luminance_moment1 * weight;
		    total_luminance_moment2 += spatial_sample.luminance_moment2 * weight;
		    total_weight += weight;
		}
	    }
	}
	total_luminance_moment1 /= total_weight;
	total_luminance_moment2 /= total_weight;
	blended_sample.variance = total_luminance_moment2 - total_luminance_moment1 * total_luminance_moment1;
    }
    
    set_new_lighting(blended_sample.lighting, pixel_coord);
    set_new_history(blended_sample, pixel_coord);
}
