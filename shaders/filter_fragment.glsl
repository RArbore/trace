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
    vec4 new_color = imageLoad(ray_tracing_output_image, pixel_coord);
    vec4 old_color = imageLoad(last_frame_image, pixel_coord);

    vec4 blended_color = mix(old_color, new_color, alpha);
    imageStore(last_frame_image, pixel_coord, blended_color);
    out_color = blended_color;
}
