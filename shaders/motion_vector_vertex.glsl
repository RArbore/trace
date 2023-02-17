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
#pragma shader_stage(vertex)
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_texture;
layout(location = 3) in mat4 in_model;

layout(location = 0) out vec4 out_new_position;
layout(location = 1) out vec4 out_old_position;

void main() {
    mat4 corrected_model = in_model;
    corrected_model[3][3] = 1.0;
    
    gl_Position = /* taa_jitter * */ perspective * camera * corrected_model * vec4(in_position, 1.0);
    out_new_position = perspective * camera * corrected_model * vec4(in_position, 1.0);
    out_old_position = perspective * last_frame_camera * corrected_model * vec4(in_position, 1.0);
}
