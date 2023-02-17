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

layout(location = 0) in vec4 in_new_position;
layout(location = 1) in vec4 in_old_position;

layout(location = 0) out vec4 out_motion_vector;

void main() {
    vec2 new_device_coord = in_new_position.xy / in_new_position.w;
    vec2 old_device_coord = in_old_position.xy / in_old_position.w;
    vec2 pixel_velocity = device_coord_to_pixel_coord(new_device_coord) - device_coord_to_pixel_coord(old_device_coord);
    out_motion_vector = vec4(pixel_velocity, 0.0, 1.0);
}
