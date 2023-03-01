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

float blur_kernel_5x5[25] = float[25](
				      1.0 / 273.0, 4.0 / 273.0, 7.0 / 273.0, 4.0 / 273.0, 1.0 / 273.0,
				      4.0 / 273.0, 16.0 / 273.0, 26.0 / 273.0, 16.0 / 273.0, 4.0 / 273.0,
				      7.0 / 273.0, 26.0 / 273.0, 41.0 / 273.0, 26.0 / 273.0, 7.0 / 273.0,
				      4.0 / 273.0, 16.0 / 273.0, 26.0 / 273.0, 16.0 / 273.0, 4.0 / 273.0,
				      1.0 / 273.0, 4.0 / 273.0, 7.0 / 273.0, 4.0 / 273.0, 1.0 / 273.0
				      );

float blur_kernel_3x3[9] = float[9](
				    1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0,
				    1.0 / 8.0, 1.0 / 4.0, 1.0 / 8.0,
				    1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0
				    );

void main() {
    vec2 pixel_coord = vec2(gl_GlobalInvocationID.xy) - 0.5;
    vec2 texture_size = textureSize(motion_vector_texture, 0);
    if (gl_GlobalInvocationID.x >= texture_size.x || gl_GlobalInvocationID.y >= texture_size.y) {
	return;
    }

    pixel_sample new_sample = get_new_sample(pixel_coord);
    vec3 atrous_lighting = vec3(0.0);
    float total_weight = 0.0;
    for (int i = -1; i <= 1; ++i) {
	for (int j = -1; j <= 1; ++j) {
	    ivec2 offset = ivec2(i, j) * (1 << (bool(temporal) ? filter_iter - 1 : filter_iter));
	    vec2 sample_pixel_coord = pixel_coord + offset;
	    if (sample_pixel_coord.x >= 0 && sample_pixel_coord.x >= 0 && sample_pixel_coord.x < texture_size.x && sample_pixel_coord.y < texture_size.y) {
	    pixel_sample blur_sample = get_new_sample(sample_pixel_coord);
	    
	    vec3 normal_dist = new_sample.normal - blur_sample.normal;
	    float normal_dist2 = dot(normal_dist, normal_dist);
	    float normal_weight = min(exp(-normal_dist2 / sigma_normal), 1.0);
	    
	    vec3 position_dist = new_sample.position - blur_sample.position;
	    float position_dist2 = dot(position_dist, position_dist);
	    float position_weight = min(exp(-position_dist2 / sigma_position), 1.0);
	    
	    float weight = normal_weight * position_weight * blur_kernel_3x3[(i + 1) * 3 + j + 1];
	    atrous_lighting += blur_sample.lighting * weight;
	    total_weight += weight;
	    }
	}
    }
    vec3 new_lighting = total_weight == 0.0 ? vec3(0.0) : atrous_lighting / total_weight;
    set_new_lighting(new_lighting, pixel_coord);
    set_new_history(new_sample, pixel_coord);
}
