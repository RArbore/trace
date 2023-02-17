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

float blur_kernel[25] = float[25](
				  1.0 / 273.0, 4.0 / 273.0, 7.0 / 273.0, 4.0 / 273.0, 1.0 / 273.0,
				  4.0 / 273.0, 16.0 / 273.0, 26.0 / 273.0, 16.0 / 273.0, 4.0 / 273.0,
				  7.0 / 273.0, 26.0 / 273.0, 41.0 / 273.0, 26.0 / 273.0, 7.0 / 273.0,
				  4.0 / 273.0, 16.0 / 273.0, 26.0 / 273.0, 16.0 / 273.0, 4.0 / 273.0,
				  1.0 / 273.0, 4.0 / 273.0, 7.0 / 273.0, 4.0 / 273.0, 1.0 / 273.0
				  );

void main() {
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = imageSize(ray_tracing_albedo_image);
    if (pixel_coord.x >= image_size.x || pixel_coord.y >= image_size.y) {
	return;
    }

    pixel_sample new_sample = get_new_sample(pixel_coord);
    vec3 atrous_lighting = vec3(0.0);
    float total_weight = 0.0;
    for (int i = -2; i <= 2; ++i) {
	for (int j = -2; j <= 2; ++j) {
	    ivec2 offset = ivec2(i, j) * (1 << filter_iter);
	    pixel_sample blur_sample = get_new_sample(pixel_coord + offset);
	    
	    vec3 normal_dist = new_sample.normal - blur_sample.normal;
	    float normal_dist2 = dot(normal_dist, normal_dist);
	    float normal_weight = min(exp(-normal_dist2 / sigma_normal), 1.0);
	    
	    vec3 position_dist = new_sample.position - blur_sample.position;
	    float position_dist2 = dot(position_dist, position_dist);
	    float position_weight = min(exp(-position_dist2 / sigma_position), 1.0);
	    
	    float weight = normal_weight * position_weight * blur_kernel[(i + 2) * 5 + j + 2];
	    atrous_lighting += blur_sample.lighting * weight;
	    total_weight += weight;
	}
    }
    vec3 new_lighting = atrous_lighting / total_weight;
    set_new_lighting(new_lighting, pixel_coord);
}
