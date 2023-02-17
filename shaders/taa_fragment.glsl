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
    /*
    vec2 fragment_UV = in_position * 0.5 + 0.5;
    vec2 motion_vector = texture(motion_vector_texture, fragment_UV).xy;
    
    vec2 pixel_coord = gl_FragCoord.xy;
    pixel_sample new_sample = get_new_sample(pixel_coord);
    pixel_sample reprojected_sample = get_old_sample(pixel_coord - motion_vector);
    vec3 min_lighting = vec3(FLOAT_MAX);
    vec3 max_lighting = vec3(0.0);
    for (float x = -1.0; x <= 1.0; x += 1.0) {
	for (float y = -1.0; y <= 1.0; y += 1.0) {
	    min_lighting = min(min_lighting, get_new_lighting(pixel_coord + vec2(x, y)));
	    max_lighting = max(max_lighting, get_new_lighting(pixel_coord + vec2(x, y)));
	}
    }

    bool blend =
	dot(new_sample.normal, reprojected_sample.normal) > 0.95 &&
	length(new_sample.position - reprojected_sample.position) < 0.1 &&
	clamp(reprojected_sample.lighting, min_lighting, max_lighting) == reprojected_sample.lighting;
    vec3 blended_lighting = mix(reprojected_sample.lighting, new_sample.lighting, alpha);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blend ? blended_lighting : new_sample.lighting;
    
    set_old_sample(blended_sample, pixel_coord);
    out_color = sample_to_color(blended_sample);
    */

    vec2 pixel_coord = gl_FragCoord.xy;
    pixel_sample new_sample = get_new_sample(pixel_coord);
    pixel_sample reprojected_sample = get_old_sample(pixel_coord);

    bool blend = last_frame_camera == camera;
    vec3 blended_lighting = mix(reprojected_sample.lighting, new_sample.lighting, alpha);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blend ? blended_lighting : new_sample.lighting;
    
    set_old_sample(blended_sample, pixel_coord);
    out_color = sample_to_color(blended_sample);
}
