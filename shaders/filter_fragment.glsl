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

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 pixel_coord = ivec2(gl_FragCoord.xy);

    /*vec4 new_NDC = perspective * camera * vec4(new_sample.position, 1.0);
    vec4 old_NDC = perspective * last_frame_camera * vec4(new_sample.position, 1.0);
    vec3 new_device = new_NDC.xyz / new_NDC.w;
    vec3 old_device = old_NDC.xyz / old_NDC.w;
    vec2 device_velocity = new_device.xy - old_device.xy;

    ivec2 reprojected_pixel_coord = device_coord_to_pixel_coord(pixel_coord_to_device_coord(pixel_coord) - device_velocity);
    pixel_sample reprojected_sample = get_old_sample(reprojected_pixel_coord);*/

    pixel_sample new_sample = get_new_sample(pixel_coord);
    pixel_sample old_sample = get_old_sample(pixel_coord);
    float depth = length(new_sample.position - camera_position);
    bool blend = depth < 0.8 * FAR_AWAY && camera == last_frame_camera;
    vec3 blended_lighting = mix(old_sample.lighting, new_sample.lighting, alpha);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blend ? blended_lighting : new_sample.lighting;
    
    set_old_sample(blended_sample, pixel_coord);
    out_color = sample_to_color(blended_sample);
}
