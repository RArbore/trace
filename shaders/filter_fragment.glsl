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

    pixel_sample new_sample = get_new_sample(pixel_coord);
    pixel_sample old_sample = get_old_sample(pixel_coord);
    float depth = length(new_sample.position - camera_position);
    bool blend = depth < 0.8 * FAR_AWAY;
    vec3 blended_lighting = mix(old_sample.lighting, new_sample.lighting, alpha);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blend ? blended_lighting : new_sample.lighting;
    
    set_old_sample(blended_sample, pixel_coord);
    out_color = sample_to_color(blended_sample);
}
