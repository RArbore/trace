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
#pragma shader_stage(fragment)
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 in_position;

layout(location = 0) out vec4 out_color;

void main() {
    vec2 pixel_coord = gl_FragCoord.xy;
    pixel_sample new_sample = get_new_sample(pixel_coord);

    if (bool(taa) && current_frame > 0) {
	vec2 texture_size = textureSize(motion_vector_texture, 0);
	vec2 fragment_UV = pixel_coord / texture_size;
	vec2 motion_vector = texture(motion_vector_texture, fragment_UV).xy;
	vec2 reprojected_pixel_coord = pixel_coord - motion_vector;
	pixel_sample reprojected_sample = get_old_sample(reprojected_pixel_coord, 0);

	vec4 new_color = sample_to_color(new_sample);
	vec4 reprojected_color = vec4(0.0);
	if (current_frame % 2 == 1) {
	    reprojected_color = texture(taa1_texture, reprojected_pixel_coord / texture_size);
	} else {
	    reprojected_color = texture(taa2_texture, reprojected_pixel_coord / texture_size);
	}
	
	bool blend =
	    dot(new_sample.normal, reprojected_sample.normal) > 0.95 &&
	    length(new_sample.position - reprojected_sample.position) < 0.5 &&
	    length(new_sample.position) < FAR_AWAY * 0.5 &&
	    length(reprojected_sample.position) < FAR_AWAY * 0.5 &&
	    reprojected_pixel_coord.x >= 0 &&
	    reprojected_pixel_coord.y >= 0 &&
	    reprojected_pixel_coord.x < texture_size.x &&
	    reprojected_pixel_coord.y < texture_size.y;
	
	vec4 blended_color = mix(reprojected_color, new_color, alpha);
	out_color = blend ? blended_color : new_color;
	if (current_frame % 2 == 0) {
	    imageStore(taa1_image, ivec2(pixel_coord), out_color);
	} else {
	    imageStore(taa2_image, ivec2(pixel_coord), out_color);
	}
    } else {
	out_color = sample_to_color(new_sample);
	if (current_frame % 2 == 0) {
	    imageStore(taa1_image, ivec2(pixel_coord), out_color);
	} else {
	    imageStore(taa2_image, ivec2(pixel_coord), out_color);
	}
    }
}
